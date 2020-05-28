/*
Copyright (C) 2006 - 2017 Evan Teran
                          evan.teran@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SessionManager.h"
#include "IPlugin.h"
#include "edb.h"
#include "Comment.h"
#include "Label.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>

#include <algorithm>

namespace {

constexpr int SessionFileVersion = 1;
const auto SessionFileIdString   = QLatin1String("edb-session");

}

/**
 * @brief SessionManager::instance
 * @return
 */
SessionManager &SessionManager::instance() {
	static SessionManager inst;
	return inst;
}

/**
 * @brief SessionManager::loadSession
 * @param filename
 * @return
 */
Result<void, SessionError> SessionManager::loadSession(const QString &filename) {

	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		// the checks for open() and exists() are racy, but
		// this is fine as it affects only whether we show an
		// error message or not
		if (!file.exists()) {
			// file doesn't exists means this is the first time we
			// opened the debuggee and there is no session file for
			// it yet

			// return success as we don't want showing error message
			// in this case
			return {};
		} else {
			SessionError session_error;
			session_error.err     = SessionError::InvalidSessionFile;
			session_error.message = tr("Failed to open session file. %1").arg(file.errorString());
			return make_unexpected(session_error);
		}
	}

	QByteArray json = file.readAll();
	QJsonParseError error;
	auto doc = QJsonDocument::fromJson(json, &error);
	if (error.error != QJsonParseError::NoError) {
		SessionError session_error;
		session_error.err     = SessionError::UnknownError;
		session_error.message = tr("An error occured while loading session JSON file. %1").arg(error.errorString());
		return make_unexpected(session_error);
	}

	if (!doc.isObject()) {
		SessionError session_error;
		session_error.err     = SessionError::NotAnObject;
		session_error.message = tr("Session file is invalid. Not an object.");
		return make_unexpected(session_error);
	}

	QJsonObject object = doc.object();
	sessionData_       = object.toVariantMap();

	QString id  = sessionData_["id"].toString();
	QString ts  = sessionData_["timestamp"].toString();
	int version = sessionData_["version"].toInt();

	Q_UNUSED(ts)

	if (id != SessionFileIdString || version > SessionFileVersion) {
		SessionError session_error;
		session_error.err     = SessionError::InvalidSessionFile;
		session_error.message = tr("Session file is invalid.");
		return make_unexpected(session_error);
	}

	qDebug("Loading session file");
	loadPluginData(); //First, load the plugin-data

	/* Restore objects (comments, labels, breakpoints, etc...) */
	QVariantMap objects = sessionData_["objects"].toMap();
	for (auto it = objects.begin(); it != objects.end(); ++it) {
		SessionObject *obj;
		edb::address_t addr;
		QVariantMap data = it.value().toMap();

		addr = edb::v1::string_to_address(it.key()).value();
		if (data["type"].toString().compare("label") == 0) {
			obj = new Label(addr, data["label"].toString());
		} else if (data["type"].toString().compare("comment") == 0) {
			obj = new Comment(addr, data["comment"].toString());
		} else {
			obj = nullptr;
			qDebug("Unknown session object type");
		}

		if (obj) {
			obj->setModule(data["module"].toString());
			this->add(obj);
		}
	}
	return {};
}

/**
 * @brief SessionManager::saveSession
 * @param filename
 */
void SessionManager::saveSession(const QString &filename) {

	qDebug("Saving session file");

	QVariantMap plugin_data, object_data;

	for (QObject *plugin : edb::v1::plugin_list()) {
		if (auto p = qobject_cast<IPlugin *>(plugin)) {
			if (const QMetaObject *const meta = plugin->metaObject()) {
				QString name     = meta->className();
				QVariantMap data = p->saveState();

				if (!data.empty()) {
					plugin_data[name] = data;
				}
			}
		}
	}

	sessionData_["version"]     = SessionFileVersion;
	sessionData_["id"]          = SessionFileIdString; // just so we can sanity check things
	sessionData_["timestamp"]   = QDateTime::currentDateTimeUtc();
	sessionData_["plugin-data"] = plugin_data;

	/* Save session objects */
	std::for_each(objects_.begin(), objects_.end(), [&object_data](SessionObject *obj)
	{
		QVariantMap res;
		edb::address_t offset;
		const auto region = obj->region();

		res = obj->save();
		res["type"] = obj->type();
		res["module"] = region->name();
		offset = obj->address() - region->start() + region->base();
		object_data[offset.toHexString()] = res;
	});

	sessionData_["objects"] = object_data;

	auto object = QJsonObject::fromVariantMap(sessionData_);
	QJsonDocument doc(object);

	QByteArray json = doc.toJson();
	QFile file(filename);

	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		file.write(json);
	}

	/* Cleanup */
	objects_.clear();
}

/**
 * @brief SessionManager::loadPluginData
 */
void SessionManager::loadPluginData() {

	qDebug("Loading plugin-data");

	QVariantMap plugin_data = sessionData_["plugin-data"].toMap();
	for (auto it = plugin_data.begin(); it != plugin_data.end(); ++it) {
		for (QObject *plugin : edb::v1::plugin_list()) {
			if (auto p = qobject_cast<IPlugin *>(plugin)) {
				if (const QMetaObject *const meta = plugin->metaObject()) {
					QString name     = meta->className();
					QVariantMap data = it.value().toMap();

					if (name == it.key()) {
						p->restoreState(data);
						break;
					}
				}
			}
		}
	}
}

/**
 * @brief SessionManager::comments
 * @return all comments from the sessionData_
 */
QVariantList SessionManager::comments() const {
	QVariantList objects;

	std::for_each(objects_.begin(), objects_.end(), [&objects](SessionObject *obj) {
		if (obj->type().compare("comment") == 0) {
			emit obj->onObjectRestored();
			objects.push_back(obj->restore());
		}
	});

	return objects;
}

/**
 * @brief SessionManager::labels
 * @return all labels from the sessionData_
 */
QVariantList SessionManager::labels() const {
	QVariantList objects;

	std::for_each(objects_.begin(), objects_.end(), [&objects](SessionObject *obj) {
		if (obj->type().compare("label") == 0) {
			emit obj->onObjectRestored();
			objects.push_back(obj->restore());
		}
	});

	return objects;
}

void SessionManager::add(SessionObject *obj) {
	//connect(this, SIGNAL(onModuleLoaded(const QString&)), obj, SLOT(restore(const QString&)));
	connect(obj, SIGNAL(onObjectRestored()), obj, SLOT(setAddress()));
	objects_.push_back(obj);
}

void SessionManager::remove(SessionObject *obj) {

}
