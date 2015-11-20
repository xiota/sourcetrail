#include "settings/ColorScheme.h"

std::shared_ptr<ColorScheme> ColorScheme::s_instance;

std::shared_ptr<ColorScheme> ColorScheme::getInstance()
{
	if (!s_instance)
	{
		s_instance = std::shared_ptr<ColorScheme>(new ColorScheme());
	}

	return s_instance;
}

ColorScheme::~ColorScheme()
{
}

std::string ColorScheme::getColor(const std::string& key) const
{
	return getValue<std::string>(key, "");
}

std::string ColorScheme::getNodeTypeColor(Node::NodeType type, const std::string& key, ColorState state) const
{
	return getNodeTypeColor(Node::getTypeString(type), key, state);
}

std::string ColorScheme::getNodeTypeColor(const std::string& typeStr, const std::string& key, ColorState state) const
{
	std::string type = getValue<std::string>("graph/node/" + typeStr + "/like", typeStr);
	std::string color = getValue<std::string>("graph/node/" + type + "/" + key + "/" + stateToString(state), "");

	if (!color.size() && state != NORMAL)
	{
		color = getValue<std::string>("graph/node/" + type + "/" + key + "/" + stateToString(NORMAL), "");
	}

	if (!color.size())
	{
		color = getValue<std::string>("graph/node/default/" + key + "/" + stateToString(state), "");
	}

	if (!color.size() && state != NORMAL)
	{
		color = getValue<std::string>("graph/node/default/" + key + "/" + stateToString(NORMAL), "#FFFFFF");
	}

	return color;
}

std::string ColorScheme::getEdgeTypeColor(Edge::EdgeType type, ColorState state) const
{
	return getEdgeTypeColor(Edge::getTypeString(type), state);
}

std::string ColorScheme::getEdgeTypeColor(const std::string& typeStr, ColorState state) const
{
	std::string type = getValue<std::string>("graph/edge/" + typeStr + "/like", typeStr);
	std::string color = getValue<std::string>("graph/edge/" + type + "/" + stateToString(state), "");

	if (!color.size() && state != NORMAL)
	{
		color = getValue<std::string>("graph/edge/" + type + "/" + stateToString(NORMAL), "");
	}

	if (!color.size())
	{
		color = getValue<std::string>("graph/edge/default/" + stateToString(state), "");
	}

	if (!color.size() && state != NORMAL)
	{
		color = getValue<std::string>("graph/edge/default/" + stateToString(NORMAL), "#FFFFFF");
	}

	return color;
}

std::string ColorScheme::getSearchTypeColor(const std::string& searchTypeName, const std::string& state) const
{
	std::string path = "search/query/" + searchTypeName + "/" + state;
	return getValue<std::string>(path, "#FFFFFF");
}

std::string ColorScheme::getSyntaxColor(const std::string& key) const
{
	return getValue<std::string>("code/snippet/syntax/" + key, "#FFFFFF");
}

ColorScheme::ColorScheme()
{
}

std::string ColorScheme::stateToString(ColorState state)
{
	switch (state)
	{
	case NORMAL: return "normal";
	case FOCUS: return "focus";
	}

	return "";
}
