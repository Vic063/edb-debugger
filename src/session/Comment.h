#ifndef COMMENT_H
#define COMMENT_H

#include "Types.h"
#include "SessionObject.h"

class Comment : public SessionObject
{
public:
	Comment(edb::address_t address, const QString &comment);
	QVariantMap save() override;
	QVariantMap restore() override;

public slots:
	void update() override;

protected:
	QString comment_;
};

#endif // COMMENT_H
