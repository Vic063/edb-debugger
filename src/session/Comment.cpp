#include "Comment.h"
#include <iostream>

Comment::Comment(edb::address_t address, const QString &comment)
{
	comment_ = comment;
	address_ = address;
	type_ = "comment";
}

QVariantMap Comment::save() {
	std::cout << "Saving comment." << std::endl;
	QVariantMap res;

	res["comment"] = comment_;

	return res;
}

QVariantMap Comment::restore() {
	QVariantMap res;

	res["comment"] = comment_;
	res["address"] = address_.toHexString();

	return res;
}

void Comment::update() {
	std::cout << "Updating comment." << std::endl;
}
