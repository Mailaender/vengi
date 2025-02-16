/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/IComponent.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "ModifierType.h"
#include "ModifierButton.h"
#include "math/AABB.h"
#include "Selection.h"
#include "ModifierVolumeWrapper.h"

namespace voxedit {

enum ShapeType {
	AABB,
	Torus,
	Cylinder,
	Cone,
	Dome,
	Ellipse,

	Max
};

static constexpr const char* ShapeTypeStr[ShapeType::Max] {
	"AABB",
	"Torus",
	"Cylinder",
	"Cone",
	"Dome",
	"Ellipse"
};

/**
 * @brief This class is responsible for manipulating the volume with the configured shape and for
 * doing the selection.
 *
 * There are several modes availalbe. E.g. having the starting point of the aabb on a corner - or
 * at the center, mirroring the modifications and so on.
 */
class Modifier : public core::IComponent {
protected:
	Selection _selection = voxel::Region::InvalidRegion;
	bool _selectionValid = false;
	bool _secondPosValid = false;
	bool _aabbMode = false;
	bool _center = false;
	/**
	 * timer value which indicates the next execution time in case you keep the
	 * modifier triggered
	 */
	double _nextSingleExecution = 0;

	glm::ivec3 _aabbFirstPos {0};
	glm::ivec3 _aabbSecondPos {0};

	/**
	 * if the current modifier type allows or needs a second action to span the
	 * volume to operate in, this is the direction into which the second action
	 * points
	 */
	math::Axis _aabbSecondActionDirection = math::Axis::None;

	ModifierType _modifierType = ModifierType::Place;

	int _gridResolution = 1;

	math::Axis _mirrorAxis = math::Axis::None;
	glm::ivec3 _mirrorPos {0};

	glm::ivec3 _cursorPosition {0};
	glm::ivec3 _referencePos;

	/** the face where the trace hit */
	voxel::FaceNames _face = voxel::FaceNames::Max;

	/** the voxel that should get placed */
	voxel::Voxel _cursorVoxel;
	/** existing voxel under the cursor */
	voxel::Voxel _hitCursorVoxel;
	/** the voxel where the cursor is - can be air */
	voxel::Voxel _voxelAtCursor;

	ModifierButton _actionExecuteButton;
	ModifierButton _deleteExecuteButton;
	ShapeType _shapeType = ShapeType::AABB;

	glm::ivec3 firstPos() const;
	bool getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const;
	math::Axis getSizeAndHeightFromAxisAndDim(math::Axis axis, const glm::ivec3& dimensions, double &size, double &height) const;
	bool executeShapeAction(ModifierVolumeWrapper& wrapper, const glm::ivec3& mins, const glm::ivec3& maxs, const std::function<void(const voxel::Region& region, ModifierType type)>& callback);
public:
	Modifier();

	/**
	 * @brief Create a Raw Volume Wrapper object while taking the selection into account
	 */
	voxel::RawVolumeWrapper createRawVolumeWrapper(voxel::RawVolume* volume) const;
	voxel::Region createRegion(const voxel::RawVolume* volume) const;

	void construct() override;
	bool init() override;
	void update(double nowSeconds);

	void shutdown() override;

	virtual bool select(const glm::ivec3& mins, const glm::ivec3& maxs);
	virtual void unselect();

	void translate(const glm::ivec3& v);

	/**
	 * @return @c true if the modifier aabb selection is not yet done, but
	 * active already
	 */
	bool secondActionMode() const;
	/**
	 * @return The axis along which the aabb may still be modified
	 */
	math::Axis secondActionDirection() const;
	/**
	 * @return @c true if the aabb that was formed has a side that is only 1 voxel
	 * high. This is our indicator for allowing to modify the aabb according to
	 * it's detected axis
	 * @sa secondActionDirection()
	 */
	bool needsSecondAction();
	const Selection& selection() const;

	/**
	 * @brief The modifier can build the aabb from the center of the current
	 * cursor position.
	 * Set this to @c true to activate this. The default is to build the aabb
	 * from the corner(s)
	 */
	void setCenterMode(bool center);
	bool centerMode() const;

	math::Axis mirrorAxis() const;
	virtual bool setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos);

	ModifierType modifierType() const;
	void setModifierType(ModifierType type);

	void setPlaneMode(bool state);
	bool planeMode() const;

	void setSingleMode(bool state);
	bool singleMode() const;

	const voxel::Voxel& cursorVoxel() const;
	virtual void setCursorVoxel(const voxel::Voxel& voxel);

	ShapeType shapeType() const;
	void setShapeType(ShapeType type);

	bool aabbMode() const;
	glm::ivec3 aabbDim() const;
	glm::ivec3 aabbPosition() const;
	math::AABB<int> aabb() const;

	/**
	 * @brief Pick the start position of the modifier execution bounding box
	 */
	bool aabbStart();
	/**
	 * @brief End the current ModifierType execution and modify tha given volume according to the type.
	 * @param[out,in] volume The volume to modify
	 * @param callback Called for every region that was modified for the current active modifier.
	 */
	bool aabbAction(voxel::RawVolume* volume, const std::function<void(const voxel::Region& region, ModifierType type)>& callback);
	void aabbAbort();
	void aabbStep();

	bool modifierTypeRequiresExistingVoxel() const;

	/**
	 * @param[in] pos The position inside the volume - given in absolute world coordinates
	 * @param[in] face The voxel::FaceNames values where the trace hits an existing voxel
	 */
	void setCursorPosition(const glm::ivec3& pos, voxel::FaceNames face);
	const glm::ivec3& cursorPosition() const;

	const glm::ivec3& referencePosition() const;
	void setReferencePosition(const glm::ivec3& pos);

	const voxel::Voxel& hitCursorVoxel() const;
	void setHitCursorVoxel(const voxel::Voxel&);
	void setVoxelAtCursor(const voxel::Voxel& voxel);

	voxel::FaceNames cursorFace() const;

	void setGridResolution(int resolution);

	void reset();
};

inline const voxel::Voxel& Modifier::hitCursorVoxel() const {
	return _hitCursorVoxel;
}

inline void Modifier::setReferencePosition(const glm::ivec3 &pos) {
	_referencePos = pos;
}

inline const glm::ivec3& Modifier::referencePosition() const {
	return _referencePos;
}

inline ModifierType Modifier::modifierType() const {
	return _modifierType;
}

inline bool Modifier::planeMode() const {
	return (_modifierType & ModifierType::Plane) == ModifierType::Plane;
}

inline bool Modifier::singleMode() const {
	return (_modifierType & ModifierType::Single) == ModifierType::Single;
}

inline bool Modifier::aabbMode() const {
	return _aabbMode;
}

inline bool Modifier::secondActionMode() const {
	return _secondPosValid;
}

inline math::Axis Modifier::secondActionDirection() const {
	return _aabbSecondActionDirection;
}

inline bool Modifier::centerMode() const {
	return _center;
}

inline void Modifier::setCenterMode(bool center) {
	_center = center;
}

inline ShapeType Modifier::shapeType() const {
	return _shapeType;
}

inline void Modifier::setShapeType(ShapeType type) {
	_shapeType = type;
}

inline math::Axis Modifier::mirrorAxis() const {
	return _mirrorAxis;
}

inline void Modifier::setCursorPosition(const glm::ivec3& pos, voxel::FaceNames face) {
	_cursorPosition = pos;
	_face = face;
}

inline voxel::FaceNames Modifier::cursorFace() const {
	return _face;
}

inline void Modifier::setCursorVoxel(const voxel::Voxel &voxel) {
	if (voxel::isAir(voxel.getMaterial())) {
		return;
	}
	_cursorVoxel = voxel;
}

inline const voxel::Voxel& Modifier::cursorVoxel() const {
	return _cursorVoxel;
}

inline const glm::ivec3& Modifier::cursorPosition() const {
	return _cursorPosition;
}

inline const Selection& Modifier::selection() const {
	return _selection;
}

}
