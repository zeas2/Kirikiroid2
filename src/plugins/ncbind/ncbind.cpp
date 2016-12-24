#include "ncbind.hpp"
#include <set>

//---------------------------------------------------------------------------
// static変数の実体

// auto register 先頭ポインタ
ncbAutoRegister::ThisClassT const*
ncbAutoRegister::_top[ncbAutoRegister::LINE_COUNT] = NCB_INNER_AUTOREGISTER_LINES_INSTANCE;

std::set<ttstr> TVPRegisteredPlugins;

std::map<ttstr, ncbAutoRegister::INTERNAL_PLUGIN_LISTS > ncbAutoRegister::_internal_plugins;

bool ncbAutoRegister::LoadModule(const ttstr &_name)
{
	ttstr name = _name.AsLowerCase();
	if (TVPRegisteredPlugins.find(name) != TVPRegisteredPlugins.end())
		return false;
	auto it = _internal_plugins.find(name);
	if (it != _internal_plugins.end()) {
		for (int line = 0; line < ncbAutoRegister::LINE_COUNT; ++line) {
			const std::list<ncbAutoRegister const*> &plugin_list = it->second.lists[line];
			for (auto i = plugin_list.begin(); i != plugin_list.end(); ++i) {
				(*i)->Regist();
			}
		}
		TVPRegisteredPlugins.insert(name);
		return true;
	}
	return false;
}