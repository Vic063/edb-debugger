#include "Label.h"
#include <iostream>

Label::Label(edb::address_t address, const QString &label)
	: Comment(address, label)
{
	type_ = "label";
}

QVariantMap Label::save() {
	std::cout << "Saving label." << std::endl;
	QVariantMap res;

	res["label"] = comment_;

	return res;
}

QVariantMap Label::restore() {
	QVariantMap res;

	res["label"] = comment_;
	res["address"] = address_.toHexString();

	return res;
}
