#ifndef WINTEX_STRUCTS_H
#define WINTEX_STRUCTS_H

#include <common/str.h>
#include <common/types.h>
#include "wintex/enums.h"
#include "wintex/point.h"

namespace WinTex {

struct Line {
	DPoint P1;
	DPoint P2;
};

struct TLPoint {
	struct Point *Point;
	float U;
	float V;

	int ObjectIndex;
	int SubObjectIndex;
	int SubObjectId;
};

struct PointUV {
	float x;
	float y;
	float u;
	float v;
};

struct Triangle {
	int ObjectId;
	int SubObjectId;
	int Flags;

	TLPoint P1;
	TLPoint P2;
	TLPoint P3;
};

struct Box {
	float X1;
	float Y1;
	float Z1;
	float X2;
	float Y2;
	float Z2;
};

struct ModelSubObject {
	int ModelIndex;
	int SubObjectIndex;
	int Flags;
	int Texture;
	int ID;
	int VerticeOffset;
	int PointCount;
	Box BoundingBox;
	bool Active;

	TLPoint *Points;
	Triangle *Triangles;
};

struct ModelObject {
	int Index;
	int SubObjectCount;
	Box BoundingBox;
	ModelSubObject *SubObjects;
};

struct SpritePosInfo {
	Point Position;
	bool Visible;
};

struct Buffer {
	byte *pData;
	int Size;
	int Frame;
};

struct Size {
	float Width;
	float Height;
};

struct Rect {
	float Top;
	float Left;
	float Bottom;
	float Right;
};

struct ListBoxItem {
	int Id = 0;
	Common::String Text = "";
	int StartVertex = 0;
	int VerticeCount = 0;
	bool MouseOver = false;
};

struct InventoryItem {
	int Id;
	float ImageX1;
	float ImageY1;
	float ImageX2;
	float ImageY2;
	float NameX1;
	float NameY1;
	float NameX2;
	float NameY2;
};

struct Animation {
	int Type; // 1,2,3,4,14,15,16 (4 can have sub-type 1,2,3,4,5,6,7,8)
	AnimationStatus Status;
	uint32 FrameDuration;
	uint32 ConstantFrameDuration;
	uint64 FrameTime;
	byte *AnimDataPointer;
	byte *AnimDataPointerInit;
	byte *AnimDataPointerEnd;
	int ParentAnim; // Used by 4.7, resume parent when this completes
	int ObjectId;
	int Parameter;
	int FrameCounter;
};

struct FrameData {
	byte *VideoPointer;
	int VideoSize;
	byte *PalettePointer;
	int PaletteSize;
	byte *AudioPointer;
	int AudiSize;

	int FrameNumber;
};

struct ControlCoordinates {
	byte KeyCode;
	byte Flags;
	int MinY;
	int MaxY;
	int MinX;
	int MaxX;
	int Action;
};

struct ControlTable {
	int ImageIndex;
	int X;
	int Y;
};

} // End of namespace WinTex

#endif // WINTEX_STRUCTS_H
