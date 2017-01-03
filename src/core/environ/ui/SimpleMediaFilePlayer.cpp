#include "SimpleMediaFilePlayer.h"
#ifdef PixelFormat // libavcodec vs cocos2d
#undef PixelFormat
#endif
#include "cocos2d.h"
#include "StorageIntf.h"
#include "SysInitIntf.h"
#include "ui/UIButton.h"
#include "ui/UIText.h"
#include "ui/UISlider.h"
#include "cocos2d/MainScene.h"
#include "cocos/ui/UIHelper.h"
#include "movie/ffmpeg/KRMoviePlayer.h"
#include "StorageImpl.h"

using namespace cocos2d;
using namespace cocos2d::ui;
void TVPControlAdDialog(int adType, int arg1, int arg2);

class SimplePlayerOverlay {
	KRMovie::VideoPresentOverlay2 *_overlay;
	tTVPRect _rect;
	const tTVPRect &GetBounds() {
		return _rect;
	}
public:
	SimplePlayerOverlay() {
		_overlay = KRMovie::VideoPresentOverlay2::create();
		_overlay->SetFuncGetBounds(std::bind(&SimplePlayerOverlay::GetBounds, this));
	}
	~SimplePlayerOverlay() {
		_overlay->Release();
	}
	void SetBounds(const tTVPRect& rect) { _rect = rect; }
	void OpenFromStream(IStream *stream, const tjs_char * streamname, const tjs_char *type, uint64_t size) {
		_overlay->GetPlayer()->OpenFromStream(stream, streamname, type, size);
	}
	void SetCallback(const std::function<void(KRMovieEvent, void*)>& func) {
		_overlay->GetPlayer()->SetCallback(func);
	}
	cocos2d::Node *GetRootNode() { return _overlay->GetRootNode(); }
	void InitRootNode(cocos2d::Node *parent) {
		cocos2d::Node *rootNode = cocos2d::Node::create();
		_overlay->SetRootNode(rootNode);
		parent->addChild(rootNode);
		rootNode->setContentSize(cocos2d::Size::ZERO);
		_overlay->SetVisible(true);
	}
	KRMovie::VideoPresentOverlay2 &GetOverlay() { return *_overlay; }
};

// hh:mm:ss
static std::string _formatTime(unsigned int t, char prefix = 0) {
	char tmp[32];
	tmp[15] = 0;
	char *p = &tmp[15];

	static const char numtbl[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	// second
	*--p = numtbl[t % 10]; t /= 10;
	*--p = numtbl[t % 6]; t /= 6;
	*--p = ':';

	// minute
	*--p = numtbl[t % 10]; t /= 10;
	*--p = numtbl[t % 6]; t /= 6;
	*--p = ':';

	// hour
	do {
		*--p = numtbl[t % 10]; t /= 10;
	} while (t);

	if (prefix) *--p = prefix;

	return p;
}

SimpleMediaFilePlayer::SimpleMediaFilePlayer()
{
	_player = new SimplePlayerOverlay;
	_player->SetCallback(std::bind(&SimpleMediaFilePlayer::onPlayerEvent,
		this, std::placeholders::_1, std::placeholders::_2));
}

SimpleMediaFilePlayer::~SimpleMediaFilePlayer()
{
	if (_player) {
		delete _player, _player = NULL;
	}
	TVPControlAdDialog(0x10002, 1, 0);
}

SimpleMediaFilePlayer * SimpleMediaFilePlayer::create()
{
	SimpleMediaFilePlayer *ret = new SimpleMediaFilePlayer;
	ret->autorelease();
	ret->initFromFile(
		"ui/MediaPlayerNavi.csb",
		"ui/MediaPlayerBody.csb",
		"ui/MediaPlayerFoot.csb");
	return ret;
}

void SimpleMediaFilePlayer::PlayFile(ttstr uri)
{
	TVPControlAdDialog(0x10002, 0, 0);
	tTJSBinaryStream* stream = TVPCreateStream(uri, TJS_BS_READ);
	ttstr ext = TVPExtractStorageExt(uri);
	ttstr filename = TVPExtractStorageName(uri);
	_player->OpenFromStream(TVPCreateIStream(stream), uri.c_str(), ext.c_str(), stream->GetSize());
	int64_t totaltime;
	_player->GetOverlay().GetTotalTime(&totaltime);
	_totalTime = totaltime / 1000;
	Title->setString(filename.AsStdString());
	PlayTime->setString(_formatTime(0));
	RemainTime->setString(_formatTime(_totalTime/*, '-'*/));
	long w, h;
	_player->GetOverlay().GetVideoSize(&w, &h);
	Size size = Overlay->getContentSize();
	float scale = size.width / size.height;
	Vec2 pos;
	if (float(w) / h > scale) {
		scale = size.width / w;
		pos.x = 0;
		pos.y = (size.height - h * scale) / 2;
		size.height = h * scale;
	} else {
		scale = size.height / h;
		pos.x = (size.width - w * scale) / 2;
		pos.y = 0;
		size.width = w * scale;
	}
	_player->SetBounds(tTVPRect(pos.x, pos.y, size.width, size.height));

	Play();
}

void SimpleMediaFilePlayer::Play()
{
	hideRemain = 3;
	scheduleUpdate();
	_player->GetOverlay().Play();
	refreshPlayButtonStatus();
}

void SimpleMediaFilePlayer::onPlayerEvent(KRMovieEvent Msg, void* p)
{
	switch (Msg) {
	case KRMovieEvent::Ended:
		_player->GetRootNode()->scheduleOnce([this](float dt) {
			//_player->Stop();
			refreshPlayButtonStatus();
		}, 0, "onEnded");
 		break;
	default:
		break;
 	}
}

void SimpleMediaFilePlayer::onSliderChanged()
{
	if (_inupdate) return;
	float tscale = Timeline->getPercent() / 100;
	int curtime = _totalTime * tscale;
	_player->GetOverlay().SetPosition(curtime * 1000);
	hideRemain = 4;

	OSD->setVisible(true);
	OSD->setOpacity(255);
	OSDText->setString(_formatTime(curtime));
}

void SimpleMediaFilePlayer::rearrangeLayout()
{
	Size sceneSize = TVPMainScene::GetInstance()->getGameNodeSize();
	setContentSize(sceneSize);
	RootNode->setContentSize(sceneSize);
	ui::Helper::doLayout(RootNode);

	Size size = iTVPBaseForm::NaviBar.Root->getContentSize();
	float scale = sceneSize.width / size.width;
	iTVPBaseForm::NaviBar.Root->setAnchorPoint(Vec2(0, 1));
	iTVPBaseForm::NaviBar.Root->setScale(scale);
	iTVPBaseForm::NaviBar.Root->setPosition(Vec2(0, sceneSize.height));

	size = BottomBar.Root->getContentSize();
	scale = sceneSize.width / size.width;
	BottomBar.Root->setScale(scale);
}

void SimpleMediaFilePlayer::bindBodyController(const NodeMap &allNodes)
{
	OSD = allNodes.findController("OSD");
	OSD->setVisible(false);
	OSDText = static_cast<Text*>(allNodes.findController("OSDText"));

	Widget *overlay = allNodes.findWidget("Overlay");
	_player->InitRootNode(overlay);
	overlay->addClickEventListener([this](Ref*) {
		if (!NaviBar->isVisible()) {
			NaviBar->setVisible(true);
			NaviBar->setOpacity(0);
			NaviBar->runAction(FadeIn::create(0.3));

			ControlBar->setVisible(true);
			ControlBar->setOpacity(0);
			ControlBar->runAction(FadeIn::create(0.3));
			hideRemain = 5;
		}
	});
	this->Overlay = overlay;
}

void SimpleMediaFilePlayer::bindFooterController(const NodeMap &allNodes)
{
	ControlBar = allNodes.findController("ControlBar");

	PlayBtn = allNodes.findWidget("PlayBtn");
	PlayBtn->addClickEventListener([this](Ref*){
		TooglePlayOrPause();
	});
	PlayBtn->addTouchEventListener([this](Ref* _p, Widget::TouchEventType ev) {
		cocos2d::ui::Widget *p = static_cast<Widget*>(_p);
		switch (ev) {
		case Widget::TouchEventType::BEGAN:
			setPlayButtonHighlight(true);
			break;
		case Widget::TouchEventType::MOVED:
			setPlayButtonHighlight(p->hitTest(p->getTouchMovePosition(), Camera::getVisitingCamera(), nullptr));
			break;
		case Widget::TouchEventType::ENDED:
		case Widget::TouchEventType::CANCELED:
			setPlayButtonHighlight(false);
			break;
		default:
			break;
		}
	});
	PlayBtnNormal = allNodes.findController("PlayBtnNormal");
	PlayBtnPress = allNodes.findController("PlayBtnPress");
	PlayIconNormal = allNodes.findController("PlayIconNormal");
	PlayIconPress = allNodes.findController("PlayIconPress");
	PauseIconNormal = allNodes.findController("PauseIconNormal");
	PauseIconPress = allNodes.findController("PauseIconPress");
	PlayBtnPress->setVisible(false);
	PlayIconPress->setVisible(false);
	PauseIconPress->setVisible(false);
	//PlayBtnNormal->setVisible(false);
	//PlayIconNormal->setVisible(false);
	PauseIconNormal->setVisible(false);
}

void SimpleMediaFilePlayer::bindHeaderController(const NodeMap &allNodes)
{
	NaviBar = allNodes.findController("NaviBar");
	cocos2d::ui::Button *Back = static_cast<Button*>(allNodes.findController("Back"));
	Back->addClickEventListener([this](Ref*) {
		removeFromParent();
	});

	Title = static_cast<Text*>(allNodes.findController("Title"));
	PlayTime = static_cast<Text*>(allNodes.findController("PlayTime"));
	RemainTime = static_cast<Text*>(allNodes.findController("RemainTime"));
	Timeline = static_cast<Slider*>(allNodes.findController("Timeline"));

	Timeline->addEventListener([this](Ref*, Slider::EventType ev) {
		onSliderChanged();
	});
}

void SimpleMediaFilePlayer::Pause()
{
	_player->GetOverlay().Pause();
	unscheduleUpdate();
	refreshPlayButtonStatus();
}

void SimpleMediaFilePlayer::TooglePlayOrPause()
{
	if (_player->GetOverlay().IsPlaying()) {
		Pause();
	} else {
		Play();
	}
}

void SimpleMediaFilePlayer::update(float dt)
{
	_inupdate = true;
	inherit::update(dt);
	_player->GetOverlay().PresentPicture(dt);
	uint64_t pos;
	_player->GetOverlay().GetPosition(&pos);
	int playtime = pos / 1000;

	float ratio = float(pos) / _totalTime / 1000;
	Timeline->setPercent(ratio * 100);

	PlayTime->setString(_formatTime(playtime));
	//RemainTime->setString(_formatTime(_totalTime - playtime, '-'));

	if (NaviBar->isVisible() && hideRemain > 0) {
		hideRemain -= dt;
		if (hideRemain <= 0) {
			NaviBar->runAction(Sequence::createWithTwoActions(
				FadeOut::create(0.3),
				CallFuncN::create([](Node* p){
				p->setVisible(false);
			})));
			ControlBar->runAction(Sequence::createWithTwoActions(
				FadeOut::create(0.3),
				CallFuncN::create([](Node* p){
				p->setVisible(false);
			})));
			if (OSD->isVisible()) {
				OSD->runAction(Sequence::createWithTwoActions(
					FadeOut::create(0.3),
					CallFuncN::create([](Node* p){
					p->setVisible(false);
				})));
			}
			hideRemain = 5;
		}

// 		if (OSD->isVisible()) {
// 			OSD->runAction(Sequence::create(DelayTime::FadeOut::create(1))
// 				;
// 		} else if (NaviBar->isVisible()) {
// 			NaviBar->
// 			inProcAction = true;
// 		}
	}
	_inupdate = false;
}

void SimpleMediaFilePlayer::setPlayButtonHighlight(bool highlight)
{
	tTVPVideoStatus status;
	_player->GetOverlay().GetStatus(&status);
	if (highlight) {
		PlayBtnNormal->setVisible(false);
		PlayIconNormal->setVisible(false);
		PauseIconNormal->setVisible(false);
		PlayBtnPress->setVisible(true);
		(status == vsPlaying ? PauseIconPress : PlayIconPress)->setVisible(true);
	} else {
		PlayBtnPress->setVisible(false);
		PlayIconPress->setVisible(false);
		PauseIconPress->setVisible(false);
		PlayBtnNormal->setVisible(true);
		(status == vsPlaying ? PauseIconNormal : PlayIconNormal)->setVisible(true);
	}
}

void SimpleMediaFilePlayer::refreshPlayButtonStatus()
{
	tTVPVideoStatus status;
	_player->GetOverlay().GetStatus(&status);
	if (PlayBtnPress->isVisible()) { // highlight
		PlayIconPress->setVisible(false);
		PauseIconPress->setVisible(false);
		(status == vsPlaying ? PauseIconPress : PlayIconPress)->setVisible(true);
	} else {
		PlayIconNormal->setVisible(false);
		PauseIconNormal->setVisible(false);
		(status == vsPlaying ? PauseIconNormal : PlayIconNormal)->setVisible(true);
	}
}
