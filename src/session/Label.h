#ifndef LABEL_H
#define LABEL_H

#include "Comment.h"

class Label : public Comment
{
public:
	Label(edb::address_t address, const QString &label);
	QVariantMap save() override;
	QVariantMap restore() override;
};

#endif // LABEL_H
