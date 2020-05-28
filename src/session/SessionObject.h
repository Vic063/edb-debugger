#ifndef SESSIONOBJECT_H
#define SESSIONOBJECT_H

#include "IRegion.h"
#include "Types.h"

#include <QObject>

class SessionObject : public QObject
{
	Q_OBJECT
public:
	explicit SessionObject(QObject *parent = nullptr);
	void restore(const QString& module);
	virtual QVariantMap save()    = 0;
	virtual QVariantMap restore() = 0;
	std::shared_ptr<IRegion> region() const;
	const edb::address_t address() const;
	void setModule(const QString& module);
	const QString& type() const;

signals:
	void onObjectModified();
	void onObjectCreated();
	void onObjectDelete();
	void onObjectRestored();

public slots:
	virtual void update() = 0;
	void setAddress();

protected:
	QString type_;
	QString module_;
	edb::address_t address_;

private:
	bool is_restored;
};

#endif // SESSIONOBJECT_H
