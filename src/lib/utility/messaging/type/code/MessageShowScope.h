#ifndef MESSAGE_SHOW_SCOPE_H
#define MESSAGE_SHOW_SCOPE_H

#include "Message.h"
#include "TabId.h"
#include "types.h"

class MessageShowScope: public Message<MessageShowScope>
{
public:
	MessageShowScope(Id scopeLocationId, bool showErrors)
		: scopeLocationId(scopeLocationId), showErrors(showErrors)
	{
		setSchedulerId(TabId::currentTab());
	}

	static const std::string getStaticType()
	{
		return "MessageShowScope";
	}

	void print(std::wostream& os) const override
	{
		os << scopeLocationId;
	}

	const Id scopeLocationId;
	const bool showErrors;
};

#endif	  // MESSAGE_SHOW_SCOPE_H
