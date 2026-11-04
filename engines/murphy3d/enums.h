#ifndef MURPHY3D_ENUMS_H
#define MURPHY3D_ENUMS_H

namespace Murphy3d {

enum class VideoMode {
	kEmbedded = 0,
	kFullScreen = 1
};

enum class ConversationOption {
	kNone = 0,
	kA = 1,
	kB = 2,
	kC = 3,
	kAskAbout = 4,
	kOffer = 7,
	kAbort = 99
};

enum class InteractionMode {
	kNone = 0,
	kAskAbout = 1,
	kOffer = 2,
	kBuy = 3
};

enum class ModuleType {
	kVideo = 1,
	kLocation = 2,
	kPicture = 3,
	kCrimeLink = 4,
	kInventory = 5,
	kNewsPaper = 6,
	kTravel = 7,
	kMainMenu = 8,
	kUltraSafe = 9,
	kTornNote = 10,
	kEncodedMessage = 11,
	kShreddedNote = 12,
	kCodePanel = 13,
	kAAASafe = 14,
	kColonelsComputer = 15,
	kGRSComputer = 16,
	kFerrelette = 17,
	kStasis = 18,
	kResumeGame = 19,
	kHints = 20,
	kVidPhone = 21,
	kCrossword = 22,
	kNewGame = 23,
	kLaptop = 24,
	kDragDropPuzzle = 25
};

enum class ActionType {
	kNone = 0x00000000,
	kLook = 0x00000001,
	kMove = 0x00000002,
	kGet = 0x00000004,
	kOnOff = 0x00000008,
	kTalk = 0x00000010,
	kOpen = 0x00000020,
	kUse = 0x00000040,
	kTerminate = 0x00000800
};

enum class AnimationStatus {
	kNotStarted = 0,
	kRunning = 1,
	kOnHold = 2,
	kCompleted = 4
};

enum class Alignment {
	kDefault = 0x00,
	kLeft = 0x01,
	kRight = 0x02,
	kCenterX = 0x04,
	kTop = 0x08,
	kBottom = 0x10,
	kCenterY = 0x20,
	kScaleX = 0x40,
	kScaleY = 0x80,
	kScale = kScaleX | kScaleY,
	kCrop = 0x100
};

} // End of namespace Murphy3d

#endif // MURPHY3D_ENUMS_H
