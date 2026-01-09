#pragma once
#include <nodePath.h>
#include <pandaFramework.h>
#include <shaderBuffer.h>
#include <shader.h>
#include <omniBoundingVolume.h>

LPoint3f roundPos (const LPoint3f& pos, const int roundx, const int roundy, const int roundz);
class Grid {
	NodePath grid;
public:
	Grid ();
	void Init (const NodePath& parent, WindowFramework* window, const int count); // Initialize grid. Must be called before use, only once.
	void place (const LPoint3f& center, const int round, const float scale); // Place grid on the scene correctly (grid cells look like they stay in the same position).
};
