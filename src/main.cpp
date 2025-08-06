#include <Geode/modify/ShareCommentLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

static const std::map<std::string, int> levelLengthKeys {
	{"Tiny", 0},
	{"Short", 1},
	{"Medium", 2},
	{"Long", 3},
	{"XL", 4}
};

std::string formattedLevelComment;
std::string formattedProfileComment;
int lastLevelID = -1;

bool enabled = false;
bool autoComment = false;
bool autoProfileComment = false;
std::string preFill = "GG!";
bool includeSecondsForPlatformer = false;
std::string levelLengthMinimum = "Long";
bool enableOnPlatformer = false;
bool trackDemons = true;
bool onlyTrackRated = true;
int minimumStars = 1;
bool alsoFillForProfileComments = true;
std::string profilePreFill = "GG!";
bool notifs = false;

class $modify(MyPlayLayer, PlayLayer) {
	void setupHasCompleted() {
		log::info("resetting formattedLevelComment, formattedProfileComment, and lastLevelID");
		formattedLevelComment = "";
		formattedProfileComment = "";
		lastLevelID = -1;
		PlayLayer::setupHasCompleted();
	}
	void levelComplete() {
		const int originalPercent = m_level ? m_level->m_normalPercent.value() : -1;
		PlayLayer::levelComplete();

		if (m_isTestMode || m_isPracticeMode) return;

		GJGameLevel* level = m_level;
		if (!enabled || !level) return;

		if (originalPercent > 99) return log::info("level was completed previously, returning");
		if (level->m_levelType != GJLevelType::Saved) return log::info("level does not have a comments section, returning");

		if (Loader::get()->isModLoaded("thesillydoggo.auto-gg")) return log::info("doggo's autogg mod found, returning");

		const bool isPlat = level->isPlatformer();
		const int lengthInt = level->m_levelLength;
		const int starValue = level->m_stars.value();
		const int levelID = level->m_levelID.value();
		const std::string& levelName = level->m_levelName;
		const int attemptCount = level->m_attempts.value();
		const int attemptTime = static_cast<int>(m_timePlayed);
		const bool isDemon = level->m_demon.value() > 0 && starValue > 9;

		log::info("====================================================");
		log::info("isPlat: {}", isPlat);
		log::info("lengthInt: {}", lengthInt);
		log::info("starValue: {}", starValue);
		log::info("levelID: {}", levelID);
		log::info("levelName: {}", levelName);
		log::info("attemptCount: {}", attemptCount);
		log::info("attemptTime: {}", attemptTime);
		log::info("isDemon: {}", isDemon);
		log::info("====================================================");
		log::info("enabled: {}", enabled);
		log::info("autoComment: {}", autoComment);
		log::info("autoProfileComment: {}", autoProfileComment);
		log::info("preFill: {}", preFill);
		log::info("includeSecondsForPlatformer: {}", includeSecondsForPlatformer);
		log::info("levelLengthMinimum: {}", levelLengthMinimum);
		log::info("enableOnPlatformer: {}", enableOnPlatformer);
		log::info("trackDemons: {}", trackDemons);
		log::info("onlyTrackRated: {}", onlyTrackRated);
		log::info("minimumStars: {}", minimumStars);
		log::info("alsoFillForProfileComments: {}", alsoFillForProfileComments);
		log::info("profilePreFill: {}", profilePreFill);
		log::info("====================================================");

		if (attemptCount < 1 || level->m_normalPercent.value() < 100) return log::info("user has frozen attempt counts or safe mode on a level they've never beaten before, returning");

		if (!enableOnPlatformer && isPlat) return log::info("level is platformer but user does not want to track platformers, returning");

		if (levelLengthMinimum == "Don't Track Classic Levels" && !isPlat) return log::info("level is classic but user does not want to track classic levels, returning");
		if (!isPlat && levelLengthMinimum != "Track All Classic Levels" && !levelLengthKeys.contains(levelLengthMinimum) && !level->isPlatformer()) return log::info("level is classic, their levelLengthMinimum setting is {}, which is not part of levelLengthKeys variable, returning", levelLengthMinimum);
		if (!isPlat && levelLengthMinimum != "Track All Classic Levels" && lengthInt < levelLengthKeys.at(levelLengthMinimum)) return log::info("level is too short per their levelLengthMinimum setting: level->m_levelLength: {} < levelLengthKeys.at(levelLengthMinimum): {}", lengthInt, levelLengthKeys.at(levelLengthMinimum));

		if (!trackDemons && isDemon) return log::info("user has not chosen to track demon level competions, returning");
		if (onlyTrackRated && starValue < 1) return log::info("nonrated level and user only wants to track rated levels, returning");
		if (!isDemon && starValue != 0 && starValue < minimumStars) return log::info("non-demon level star/moon rate is too low per their minimumStars setting: level->m_stars.value(): {} < minimumStars: {}", starValue, minimumStars);

		std::string suffix = fmt::format("{} att", attemptCount);
		if (isPlat && includeSecondsForPlatformer) suffix = suffix.append(fmt::format(" {} sec", attemptTime));
		formattedLevelComment = fmt::format("{} ({})", preFill, suffix);
		formattedProfileComment = fmt::format("{} 100%, {} ({})", levelName, profilePreFill, suffix);
		lastLevelID = levelID;

		log::info("formattedLevelComment: {}", formattedLevelComment);
		log::info("formattedProfileComment: {}", formattedProfileComment);
		log::info("lastLevelID: {}", lastLevelID);

		GameLevelManager* glm = GameLevelManager::get();
		if (autoComment) glm->uploadComment(formattedLevelComment, CommentType::Level, levelID, 100);
		if (autoProfileComment) glm->uploadComment(formattedProfileComment, CommentType::Account, 0, 0);

		if (!notifs) return log::info("user does not want notifications, returning");

		std::string notifString = "Created completion comments";
		if (autoComment && autoProfileComment) notifString = "Uploading both completion comments";
		else if (autoComment && !autoProfileComment) notifString = "Uploading comment to level";
		else if (!autoComment && autoProfileComment) notifString = "Uploading comment to profile";

		if (!notifString.empty()) Notification::create(notifString, NotificationIcon::Info)->show();
	}
};

class $modify(MyShareCommentLayer, ShareCommentLayer) {
	bool init(gd::string title, int charLimit, CommentType type, int id, gd::string desc) {
		if (!ShareCommentLayer::init(title, charLimit, type, id, desc)) return false;
		if (!enabled || (type != CommentType::Account && type != CommentType::Level) || lastLevelID == -1) return true;

		log::info("id: {}", id);

		if (!autoComment && type == CommentType::Level && id == lastLevelID && !formattedLevelComment.empty()) {
			log::info("type: Level");
			if (m_commentInput) m_commentInput->setString(formattedLevelComment);
			// may god forgive me for this warcrime known as nullptr prevention-induced hyper indentation
			if (m_mainLayer) {
				if (const auto menu = m_mainLayer->getChildByType<CCMenu>(0)) {
					if (const auto toggler = menu->getChildByType<CCMenuItemToggler>(0)) {
						Loader::get()->queueInMainThread([this, toggler]() {
							if (toggler) toggler->activate();
						});
					}
				}
			}
		} else if (!autoProfileComment && type == CommentType::Account && !formattedProfileComment.empty()) {
			log::info("type: Account");
			if (m_commentInput) m_commentInput->setString(formattedProfileComment);
		}

		this->updateCharCountLabel();

		return true;
	}
};

$on_mod(Loaded) {
	// goddammit chloe now you got me going fuckin paranoid ;-;
	formattedLevelComment = "";
	formattedProfileComment = "";
	lastLevelID = -1;

	Mod::get()->setLoggingEnabled(Mod::get()->getSettingValue<bool>("logging"));
	listenForSettingChanges<bool>("logging", [](const bool loggingNew) {
		Mod::get()->setLoggingEnabled(loggingNew);
	});

	enabled = Mod::get()->getSettingValue<bool>("enabled");
	autoComment = Mod::get()->getSettingValue<bool>("autoComment");
	autoProfileComment = Mod::get()->getSettingValue<bool>("autoProfileComment");
	preFill = Mod::get()->getSettingValue<std::string>("preFill");
	includeSecondsForPlatformer = Mod::get()->getSettingValue<bool>("includeSecondsForPlatformer");
	levelLengthMinimum = Mod::get()->getSettingValue<std::string>("levelLengthMinimum");
	enableOnPlatformer = Mod::get()->getSettingValue<bool>("enableOnPlatformer");
	trackDemons = Mod::get()->getSettingValue<bool>("trackDemons");
	onlyTrackRated = Mod::get()->getSettingValue<bool>("onlyTrackRated");
	minimumStars = static_cast<int>(Mod::get()->getSettingValue<int64_t>("minimumStars"));
	alsoFillForProfileComments = Mod::get()->getSettingValue<bool>("alsoFillForProfileComments");
	profilePreFill = Mod::get()->getSettingValue<std::string>("profilePreFill");
	notifs = Mod::get()->getSettingValue<bool>("notifs");

	if (preFill.empty()) preFill = "GG!";
	else if (preFill.length() > 30) preFill = preFill.substr(30);

	if (profilePreFill.empty()) profilePreFill = "GG!";
	else if (profilePreFill.length() > 30) profilePreFill = profilePreFill.substr(30);
	
	listenForSettingChanges<bool>("enabled", [](const bool enabledNew) {
		enabled = enabledNew;
		log::info("enabled: {}", enabled);
	});
	listenForSettingChanges<bool>("autoComment", [](const bool autoCommentNew) {
		autoComment = autoCommentNew;
		log::info("autoComment: {}", autoComment);
	});
	listenForSettingChanges<bool>("autoProfileComment", [](const bool autoProfileCommentNew) {
		autoProfileComment = autoProfileCommentNew;
		log::info("autoProfileComment: {}", autoProfileComment);
	});
	listenForSettingChanges<std::string>("preFill", [](const std::string& preFillNew) {
		if (preFillNew.empty()) preFill = "GG!";
		else if (preFillNew.length() > 30) levelLengthMinimum = preFillNew.substr(30);
		else preFill = preFillNew;
		log::info("preFill: {}", preFill);
	});
	listenForSettingChanges<bool>("includeSecondsForPlatformer", [](const bool includeSecondsForPlatformerNew) {
		includeSecondsForPlatformer = includeSecondsForPlatformerNew;
		log::info("includeSecondsForPlatformer: {}", includeSecondsForPlatformer);
	});
	listenForSettingChanges<std::string>("levelLengthMinimum", [](const std::string& levelLengthMinimumNew) {
		levelLengthMinimum = levelLengthMinimumNew;
		log::info("levelLengthMinimumNew: {}", levelLengthMinimumNew);
	});
	listenForSettingChanges<bool>("enableOnPlatformer", [](const bool enableOnPlatformerNew) {
		enableOnPlatformer = enableOnPlatformerNew;
		log::info("enableOnPlatformer: {}", enableOnPlatformer);
	});
	listenForSettingChanges<bool>("trackDemons", [](const bool trackDemonsNew) {
		trackDemons = trackDemonsNew;
		log::info("trackDemons: {}", trackDemons);
	});
	listenForSettingChanges<bool>("onlyTrackRated", [](const bool onlyTrackRatedNew) {
		onlyTrackRated = onlyTrackRatedNew;
		log::info("onlyTrackRated: {}", onlyTrackRated);
	});
	listenForSettingChanges<int64_t>("minimumStars", [](const int64_t minimumStarsNew) {
		minimumStars = std::clamp<int>(static_cast<int>(minimumStarsNew), 1, 9);
		log::info("minimumStars: {}", minimumStars);
	});
	listenForSettingChanges<bool>("alsoFillForProfileComments", [](const bool alsoFillForProfileCommentsNew) {
		alsoFillForProfileComments = alsoFillForProfileCommentsNew;
		log::info("alsoFillForProfileComments: {}", alsoFillForProfileComments);
	});
	listenForSettingChanges<std::string>("profilePreFill", [](const std::string& profilePreFillNew) {
		if (profilePreFillNew.empty()) profilePreFill = "GG!";
		else if (profilePreFillNew.length() > 30) profilePreFill = profilePreFillNew.substr(30);
		else profilePreFill = profilePreFillNew;
		log::info("profilePreFillNew: {}", profilePreFillNew);
	});
	listenForSettingChanges<bool>("notifs", [](const bool notifsNew) {
		notifs = notifsNew;
		log::info("notifsNew: {}", notifs);
	});

	log::info("enabled: {}", enabled);
	log::info("autoComment: {}", autoComment);
	log::info("autoProfileComment: {}", autoProfileComment);
	log::info("preFill: {}", preFill);
	log::info("includeSecondsForPlatformer: {}", includeSecondsForPlatformer);
	log::info("levelLengthMinimum: {}", levelLengthMinimum);
	log::info("enableOnPlatformer: {}", enableOnPlatformer);
	log::info("trackDemons: {}", trackDemons);
	log::info("onlyTrackRated: {}", onlyTrackRated);
	log::info("minimumStars: {}", minimumStars);
	log::info("alsoFillForProfileComments: {}", alsoFillForProfileComments);
	log::info("profilePreFill: {}", profilePreFill);
	log::info("notifs: {}", notifs);
}