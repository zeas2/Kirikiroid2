#include "DebugViewLayerForm.h"
#include "extensions/GUI/CCScrollView/CCTableView.h"
#include "WindowIntf.h"
#include "DrawDevice.h"
#include "cocos2d/MainScene.h"
#include <ui/UIButton.h>
#include <2d/CCSprite.h>
#include <2d/CCLabel.h>
#include "RenderManager.h"

USING_NS_CC;
USING_NS_CC_EXT;

class DebugViewLayerForm::DebugViewLayerCell : public TableViewCell {
public:
	static DebugViewLayerCell* create() {
		DebugViewLayerCell* ret = new DebugViewLayerCell;
		ret->autorelease();
		ret->init();
		return ret;
	}

	virtual bool init() override {
		TableViewCell::init();
		_bottomLine = LayerColor::create(Color4B::WHITE, 100, 1);
		addChild(_bottomLine);
		_sprite = Sprite::create();
		_sprite->setAnchorPoint(Vec2::ZERO);
		addChild(_sprite);
		_name = Label::createWithTTF("", "DroidSansFallback.ttf", 16);
		_name->setAnchorPoint(Vec2::ZERO);
		addChild(_name);
		_memsize = Label::createWithTTF("", "DroidSansFallback.ttf", 16);
		_memsize->setAnchorPoint(Vec2(1, 0));
		addChild(_memsize);
		return true;
	}

	virtual void setContentSize(const Size& contentSize) override {
		TableViewCell::setContentSize(contentSize);
		_bottomLine->setContentSize(Size(contentSize.width, 2));
	}

	void setData(const Size &laySize, const LayerInfo &data) {
		float left = data.Indent * 5;
		iTVPTexture2D *tex = data.Texture;
		if (tex) {
			_sprite->setVisible(true);
			Texture2D* cctex = _sprite->getTexture();
			Texture2D* newtex = tex->GetAdapterTexture(cctex);
			float scalex, scaley;
			tex->GetScale(scalex, scaley);
			if (newtex != cctex) {
				_sprite->setTexture(newtex);
				float sw, sh;
				if (scalex == 1.f) sw = tex->GetWidth();
				else sw = tex->GetInternalWidth();
				if (scaley == 1.f) sh = tex->GetHeight();
				else sh = tex->GetInternalHeight();
				_sprite->setTextureRect(Rect(0, 0, sw, sh));
			}
			_sprite->setPosition(left, 2);
			float r = laySize.height / tex->GetHeight();
			_sprite->setScale(r * scalex, r * scaley);
			const char *prefix = tex->IsStatic() ? "static " : "";
			char tmp[64];
			sprintf(tmp, "%s[%d x %d] %.2fMB", prefix, (int)tex->GetWidth(), (int)tex->GetHeight(), data.VMemSize / (1024.f * 1024.f));
			_memsize->setVisible(true);
			_memsize->setString(tmp);
			_memsize->setPosition(laySize.width, laySize.height + 2);
		} else {
			_sprite->setVisible(false);
			_memsize->setVisible(false);
		}
		_name->setPosition(/*left*/0, laySize.height + 2);
		_name->setString(data.Name);
		Size newSize(laySize);
		newSize.height += 24;
		setContentSize(newSize);
	}

private:
	LayerColor *_bottomLine;
	Sprite *_sprite;
	Label *_name, *_memsize;
};

DebugViewLayerForm * DebugViewLayerForm::create() {
	DebugViewLayerForm * ret = new DebugViewLayerForm;
	ret->autorelease();
	ret->init();
	return ret;
}

class tHackTVPDrawDevice : public tTVPDrawDevice {
public:
	iTVPLayerManager *getPrimaryLayerManager() {
		return GetLayerManagerAt(PrimaryLayerManagerIndex);
	}
};

class tHackTableView : public TableView {
public:
	void setSwallowTouches(bool swallow) {
		if (_touchListener) {
			_touchListener->setSwallowTouches(swallow);
		}
	}
};

static unsigned char _2x2_block_Image[] = {
	// RGBA8888
	0x2F, 0x2F, 0x2F, 0xFF,
	0x40, 0x40, 0x40, 0xFF,
	0x40, 0x40, 0x40, 0xFF,
	0x2F, 0x2F, 0x2F, 0xFF
};

bool DebugViewLayerForm::init() {
	Node::init();
	Size selfsize = TVPMainScene::GetInstance()->getGameNodeSize();
	setContentSize(selfsize);

	Texture2D* tex = new Texture2D();
	tex->autorelease();
	tex->initWithData(_2x2_block_Image, 16, Texture2D::PixelFormat::RGBA8888, 2, 2, Size::ZERO);
	tex->setTexParameters(Texture2D::TexParams{
		GL_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT
	});
	Sprite *_backGround = Sprite::create();
	_backGround->setTexture(tex);
	_backGround->setScale(16);
	_backGround->setTextureRect(Rect(0, 0, selfsize.width / 16, selfsize.height / 16));
	//LayerColor *_backGround = LayerColor::create(Color4B(16, 16, 16, 255), selfsize.width, selfsize.height);
	_backGround->setAnchorPoint(Vec2::ZERO);
	addChild(_backGround);

	_tableView = TableView::create(this, getContentSize());
	_tableView->setVerticalFillOrder(TableView::VerticalFillOrder::TOP_DOWN);
	static_cast<tHackTableView*>(_tableView)->setSwallowTouches(true);
	addChild(_tableView);
	setOnExitCallback(std::bind(&DebugViewLayerForm::onExitCallback, this));

	_totalSize = Label::createWithTTF("", "DroidSansFallback.ttf", 16);
	_totalSize->setAnchorPoint(Vec2(1, 1));

	iTVPLayerManager *manager = static_cast<tHackTVPDrawDevice*>(TVPMainWindow->GetDrawDevice())->getPrimaryLayerManager();
	uint64_t totalSize = addToLayerVec(0, "", manager->GetPrimaryLayer());
	char tmp[32];
	sprintf(tmp, "%.2fMB", (float)((double)totalSize / (1024.f * 1024.f)));
	_totalSize->setString(tmp);
	addChild(_totalSize);

	ui::Button *btnClose = ui::Button::create("img/Cancel_Normal.png", "img/Cancel_Press.png");
	btnClose->setTouchEnabled(true);
	btnClose->addClickEventListener([this](Ref*){
		removeFromParent();
	});
	btnClose->setPosition(getContentSize() - btnClose->getContentSize());
	addChild(btnClose);
	_totalSize->setPosition(btnClose->getPosition().x, getContentSize().height);

	_tableView->reloadData();
	return true;
}

Size DebugViewLayerForm::tableCellSizeForIndex(TableView *table, ssize_t idx) {
	iTVPTexture2D *tex = _layers[idx].Texture;
	cocos2d::Size laySize(getContentSize().width, 0);
	if (tex) {
		laySize.width = tex->GetWidth();
		laySize.height = tex->GetHeight();
		float maxHeight = getContentSize().height / 2.5f;
		if (laySize.height > maxHeight) {
			laySize.height = maxHeight;
		}
	}
	laySize.height += 24;
	return laySize;
}

cocos2d::extension::TableViewCell* DebugViewLayerForm::tableCellAtIndex(cocos2d::extension::TableView *table, ssize_t idx) {
	iTVPTexture2D *tex = _layers[idx].Texture;
	cocos2d::Size laySize(getContentSize().width, 0);
	if (tex) {
		laySize.height = tex->GetHeight();
		float maxHeight = getContentSize().height / 2.5f;
		if (laySize.height > maxHeight) {
			laySize.height = maxHeight;
		}
	}

	DebugViewLayerCell *cell = static_cast<DebugViewLayerCell*>(table->dequeueCell());
	if (!cell) cell = DebugViewLayerCell::create();

	cell->setData(laySize, _layers[idx]);
	return cell;
}

void DebugViewLayerForm::onExitCallback() {
	for (const LayerInfo& info : _layers) {
		if (info.Texture) info.Texture->Release();
	}
	TVPMainScene::GetInstance()->scheduleUpdate();
}

uint64_t DebugViewLayerForm::addToLayerVec(int indent, const std::string &prefix, tTJSNI_BaseLayer* lay) {
	if (!lay) return 0;
	iTVPBaseBitmap *img = lay->GetMainImage();
	iTVPTexture2D *tex = nullptr;
	uint64_t vmemsize = 0;
	if (img) {
		tex = img->GetTexture();
		tex->AddRef();
		if (!TVPGetRenderManager()->GetTextureStat(tex, vmemsize)) vmemsize = 0;
	}
	std::string name(prefix + lay->GetName().AsStdString());
	_layers.emplace_back(LayerInfo{ name, tex, (size_t)vmemsize, indent++ });
	std::string _prefix(name);
	_prefix += "/";
	//lay->GetOwnerNoAddRef()->AddRef();
	
	tjs_int childCount = lay->GetCount();
	for (tjs_int i = 0; i < childCount; ++i) {
		vmemsize += addToLayerVec(indent, _prefix, lay->GetChildren(i));
	}

	return vmemsize;
}
