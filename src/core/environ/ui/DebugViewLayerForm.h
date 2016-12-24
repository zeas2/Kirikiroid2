#pragma once
#include "2d/CCNode.h"
#include "extensions/GUI/CCScrollView/CCTableView.h"
#include "2d/CCLabel.h"

class tTJSNI_BaseLayer;
class iTVPTexture2D;
class DebugViewLayerForm : public cocos2d::Node, public cocos2d::extension::TableViewDataSource {
public:
	static DebugViewLayerForm *create();

	virtual bool init() override;

private:
	virtual cocos2d::Size tableCellSizeForIndex(cocos2d::extension::TableView *table, ssize_t idx) override;
	virtual cocos2d::extension::TableViewCell* tableCellAtIndex(cocos2d::extension::TableView *table, ssize_t idx) override;
	virtual ssize_t numberOfCellsInTableView(cocos2d::extension::TableView *table) override { return _layers.size(); }
	void onExitCallback();
	uint64_t addToLayerVec(int indent, const std::string &prefix, tTJSNI_BaseLayer* lay);

	class DebugViewLayerCell;
	struct LayerInfo {
		std::string Name;
		iTVPTexture2D *Texture;
		size_t VMemSize;
		int Indent;
	};

	cocos2d::Label *_totalSize;
	cocos2d::extension::TableView *_tableView;
	// pair<ident, layer>
	std::vector<LayerInfo> _layers;
};