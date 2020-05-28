#include "edb.h"
#include "IRegion.h"
#include "MemoryRegions.h"
#include "SessionObject.h"

SessionObject::SessionObject(QObject *parent)
	: QObject(parent)
{
	is_restored = false;
}

std::shared_ptr<IRegion> SessionObject::region() const {
	return edb::v1::memory_regions().findRegion(address_);
}

const edb::address_t SessionObject::address() const {
	return address_;
}

void SessionObject::setModule(const QString& module) {
	module_ = module;
}

const QString& SessionObject::type() const {
	return type_;
}

void SessionObject::setAddress() {
	const auto region = edb::v1::memory_regions().findRegion(module_);
	if (is_restored) {
		return;
	}

	if (region) {
		address_ += region->start();
		is_restored = true;
	} else {
		// Defer restoration when module will be loaded
		//connect(this, SIGNAL(onModuleLoaded(QString&)), edb::v1::memory_regions(), SLOT(loadModule(QString&)));
	}
}
