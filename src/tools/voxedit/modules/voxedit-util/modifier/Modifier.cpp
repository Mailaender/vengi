/**
 * @file
 */

#include "Modifier.h"
#include "math/Axis.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "command/Command.h"
#include "ui/imgui/dearimgui/imgui_internal.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "../AxisUtil.h"
#include "../CustomBindingContext.h"
#include "../SceneManager.h"
#include "voxelutil/VoxelUtil.h"
#include "voxelutil/AStarPathfinder.h"

namespace voxedit {

Modifier::Modifier() :
				_deleteExecuteButton(ModifierType::Erase) {
}

void Modifier::construct() {
	command::Command::registerActionButton("actionexecute", _actionExecuteButton).setBindingContext(BindingContext::Model);
	command::Command::registerActionButton("actionexecutedelete", _deleteExecuteButton).setBindingContext(BindingContext::Model);

	command::Command::registerCommand("actionselect", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Select);
	}).setHelp("Change the modifier type to 'select'");

	command::Command::registerCommand("actioncolorpicker", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::ColorPicker);
	}).setHelp("Change the modifier type to 'color picker'");

	command::Command::registerCommand("actionpath", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Path);
	}).setHelp("Change the modifier type to 'path'");

	command::Command::registerCommand("actionline", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Line);
	}).setHelp("Change the modifier type to 'line'");

	command::Command::registerCommand("actionerase", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Erase);
	}).setHelp("Change the modifier type to 'erase'");

	command::Command::registerCommand("actionplace", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Place);
	}).setHelp("Change the modifier type to 'place'");

	command::Command::registerCommand("actionpaint", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Paint);
	}).setHelp("Change the modifier type to 'paint'");

	command::Command::registerCommand("actionoverride", [&] (const command::CmdArgs& args) {
		setModifierType(ModifierType::Place | ModifierType::Erase);
	}).setHelp("Change the modifier type to 'override'");

	command::Command::registerCommand("shapeaabb", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::AABB);
	}).setHelp("Change the shape type to 'aabb'");

	command::Command::registerCommand("shapetorus", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Torus);
	}).setHelp("Change the shape type to 'torus'");

	command::Command::registerCommand("shapecylinder", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Cylinder);
	}).setHelp("Change the shape type to 'cylinder'");

	command::Command::registerCommand("shapeellipse", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Ellipse);
	}).setHelp("Change the shape type to 'ellipse'");

	command::Command::registerCommand("shapecone", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Cone);
	}).setHelp("Change the shape type to 'cone'");

	command::Command::registerCommand("shapedome", [&] (const command::CmdArgs& args) {
		setShapeType(ShapeType::Dome);
	}).setHelp("Change the shape type to 'dome'");

	command::Command::registerCommand("mirroraxisx", [&] (const command::CmdArgs& args) {
		setMirrorAxis(math::Axis::X, sceneMgr().referencePosition());
	}).setHelp("Mirror around the x axis");

	command::Command::registerCommand("mirroraxisy", [&] (const command::CmdArgs& args) {
		setMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());
	}).setHelp("Mirror around the y axis");

	command::Command::registerCommand("mirroraxisz", [&] (const command::CmdArgs& args) {
		setMirrorAxis(math::Axis::Z, sceneMgr().referencePosition());
	}).setHelp("Mirror around the z axis");

	command::Command::registerCommand("mirroraxisnone", [&] (const command::CmdArgs& args) {
		setMirrorAxis(math::Axis::None, sceneMgr().referencePosition());
	}).setHelp("Disable mirror axis");
}

bool Modifier::init() {
	return true;
}

void Modifier::shutdown() {
	reset();
}

void Modifier::update(double nowSeconds) {
	if (!singleMode()) {
		return;
	}
	if (_actionExecuteButton.pressed() && nowSeconds >= _nextSingleExecution) {
		_actionExecuteButton.execute(true);
		_nextSingleExecution = nowSeconds + 0.1;
	}
}

void Modifier::reset() {
	unselect();
	_gridResolution = 1;
	_secondPosValid = false;
	_aabbMode = false;
	_center = false;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
	_aabbSecondActionDirection = math::Axis::None;
	_modifierType = ModifierType::Place;
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
	_cursorPosition = glm::ivec3(0);
	_face = voxel::FaceNames::Max;
	_shapeType = ShapeType::AABB;
	setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0));
}

glm::ivec3 Modifier::aabbPosition() const {
	glm::ivec3 pos = _cursorPosition;
	if (_secondPosValid) {
		switch (_aabbSecondActionDirection) {
		case math::Axis::X:
			pos.y = _aabbSecondPos.y;
			pos.z = _aabbSecondPos.z;
			break;
		case math::Axis::Y:
			pos.x = _aabbSecondPos.x;
			pos.z = _aabbSecondPos.z;
			break;
		case math::Axis::Z:
			pos.x = _aabbSecondPos.x;
			pos.y = _aabbSecondPos.y;
			break;
		default:
			break;
		}
	}
	return pos;
}

bool Modifier::aabbStart() {
	if (_aabbMode) {
		return false;
	}

	// the order here matters - don't change _aabbMode earlier here
	_aabbFirstPos = aabbPosition();
	_secondPosValid = false;
	_aabbSecondActionDirection = math::Axis::None;
	_aabbMode = !singleMode();
	return true;
}

void Modifier::aabbStep() {
	if (!_aabbMode) {
		return;
	}
	_aabbSecondPos = aabbPosition();
	_aabbFirstPos = firstPos();
	_secondPosValid = true;
}

bool Modifier::getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const {
	math::Axis mirrorAxis = _mirrorAxis;
	if (mirrorAxis == math::Axis::None) {
		return false;
	}
	const int index = getIndexForMirrorAxis(mirrorAxis);
	int deltaMaxs = _mirrorPos[index] - maxs[index] - 1;
	deltaMaxs *= 2;
	deltaMaxs += (maxs[index] - mins[index] + 1);
	mins[index] += deltaMaxs;
	maxs[index] += deltaMaxs;
	return true;
}

void Modifier::unselect() {
	_selection = voxel::Region::InvalidRegion;
	_selectionValid = false;
}

bool Modifier::select(const glm::ivec3 &mins, const glm::ivec3 &maxs) {
	const bool selectActive = (_modifierType & ModifierType::Erase) == ModifierType::None;
	if (selectActive) {
		_selection = voxel::Region{mins, maxs};
		_selectionValid = _selection.isValid();
	} else {
		unselect();
	}
	return true;
}

math::Axis Modifier::getSizeAndHeightFromAxisAndDim(math::Axis axis, const glm::ivec3& dimensions, double &size, double &height) const {
	if (axis == math::Axis::None) {
		axis = math::Axis::Y;
	}
	switch (axis) {
	case math::Axis::X:
		size = (glm::max)(dimensions.y, dimensions.z);
		height = dimensions.x;
		break;
	case math::Axis::Y:
		size = (glm::max)(dimensions.x, dimensions.z);
		height = dimensions.y;
		break;
	case math::Axis::Z:
		size = (glm::max)(dimensions.x, dimensions.y);
		height = dimensions.z;
		break;
	default:
		return math::Axis::None;
	}
	return axis;
}

bool Modifier::executeShapeAction(ModifierVolumeWrapper& wrapper, const glm::ivec3& mins, const glm::ivec3& maxs, const std::function<void(const voxel::Region& region, ModifierType type)>& callback) {
	glm::ivec3 operateMins = mins;
	glm::ivec3 operateMaxs = maxs;
	if (_selectionValid) {
		operateMins = (glm::max)(mins, _selection.getLowerCorner());
		operateMaxs = (glm::min)(maxs, _selection.getUpperCorner());
	}

	const voxel::Region region(operateMins, operateMaxs);
	voxel::logRegion("Shape action execution", region);
	const glm::ivec3& center = region.getCenter();
	glm::ivec3 centerBottom = center;
	centerBottom.y = region.getLowerY();
	const glm::ivec3& dimensions = region.getDimensionsInVoxels();

	double size;
	double height;
	const math::Axis axis = getSizeAndHeightFromAxisAndDim(_aabbSecondActionDirection, dimensions, size, height);

	switch (_shapeType) {
	case ShapeType::AABB:
		voxelgenerator::shape::createCubeNoCenter(wrapper, operateMins, dimensions, _cursorVoxel);
		break;
	case ShapeType::Torus: {
		const double minorRadius = size / 5.0;
		const double majorRadius = size / 2.0 - minorRadius;
		voxelgenerator::shape::createTorus(wrapper, center, minorRadius, majorRadius, _cursorVoxel);
		break;
	}
	case ShapeType::Cylinder: {
		const double radius = size / 2.0;
		voxelgenerator::shape::createCylinder(wrapper, centerBottom, axis, (int)glm::round(radius), (int)glm::round(height), _cursorVoxel);
		break;
	}
	case ShapeType::Cone:
		voxelgenerator::shape::createCone(wrapper, center, dimensions, _cursorVoxel);
		break;
	case ShapeType::Dome:
		voxelgenerator::shape::createDome(wrapper, center, dimensions, _cursorVoxel);
		break;
	case ShapeType::Ellipse:
		voxelgenerator::shape::createEllipse(wrapper, center, dimensions, _cursorVoxel);
		break;
	case ShapeType::Max:
		Log::warn("Invalid shape type selected - can't perform action");
		return false;
	}
	const voxel::Region& modifiedRegion = wrapper.dirtyRegion();
	if (modifiedRegion.isValid()) {
		voxel::logRegion("Dirty region", modifiedRegion);
		callback(modifiedRegion, _modifierType);
	}
	return true;
}

bool Modifier::needsSecondAction() {
	if (singleMode()) {
		return false;
	}

	const glm::ivec3 delta = aabbDim();
	if (delta.x > _gridResolution && delta.z > _gridResolution && delta.y == _gridResolution) {
		_aabbSecondActionDirection = math::Axis::Y;
	} else if (delta.y > _gridResolution && delta.z > _gridResolution && delta.x == _gridResolution) {
		_aabbSecondActionDirection = math::Axis::X;
	} else if (delta.x > _gridResolution && delta.y > _gridResolution && delta.z == _gridResolution) {
		_aabbSecondActionDirection = math::Axis::Z;
	} else {
		_aabbSecondActionDirection = math::Axis::None;
	}
	return _aabbSecondActionDirection != math::Axis::None;
}

glm::ivec3 Modifier::firstPos() const {
	if (!_center || _secondPosValid) {
		return _aabbFirstPos;
	}
	const int size = _gridResolution;
	const glm::ivec3& first = _aabbFirstPos;
	const glm::ivec3& pos = aabbPosition();
	const glm::ivec3& mins = (glm::min)(first, pos);
	const glm::ivec3& maxs = (glm::max)(first, pos);
	const glm::ivec3& delta = maxs + size - mins;
	const glm::ivec3& deltaa = glm::abs(delta);
	glm::ivec3 f = _aabbFirstPos;
	if (deltaa.x > 1 && deltaa.z > 1 && deltaa.y == 1) {
		f.x += delta.x;
		f.z += delta.z;
	} else if (deltaa.y > 1 && deltaa.z > 1 && deltaa.x == 1) {
		f.y += delta.y;
		f.z += delta.z;
	} else if (deltaa.x > 1 && deltaa.y > 1 && deltaa.z == 1) {
		f.x += delta.x;
		f.y += delta.y;
	}
	return f;
}

math::AABB<int> Modifier::aabb() const {
	const int size = _gridResolution;
	const glm::ivec3 &pos = aabbPosition();
	const bool single = singleMode();
	const glm::ivec3 &firstP = single ? pos : firstPos();
	const glm::ivec3 mins = (glm::min)(firstP, pos);
	const glm::ivec3 maxs = (glm::max)(firstP, pos) + (size - 1);
	return math::AABB<int>(mins, maxs);
}

glm::ivec3 Modifier::aabbDim() const {
	const int size = _gridResolution;
	glm::ivec3 pos = aabbPosition();
	const glm::ivec3& first = firstPos();
	const glm::ivec3& mins = (glm::min)(first, pos);
	const glm::ivec3& maxs = (glm::max)(first, pos);
	return glm::abs(maxs + size - mins);
}

voxel::RawVolumeWrapper Modifier::createRawVolumeWrapper(voxel::RawVolume* volume) const {
	return voxel::RawVolumeWrapper(volume, createRegion(volume));
}

voxel::Region Modifier::createRegion(const voxel::RawVolume* volume) const {
	voxel::Region region = volume->region();
	if (_selectionValid) {
		voxel::Region srcRegion(_selection);
		srcRegion.cropTo(region);
		return srcRegion;
	}
	return region;
}

void Modifier::setHitCursorVoxel(const voxel::Voxel& voxel) {
	_hitCursorVoxel = voxel;
}

void Modifier::setVoxelAtCursor(const voxel::Voxel& voxel) {
	_voxelAtCursor = voxel;
}

bool Modifier::aabbAction(voxel::RawVolume *volume,
						  const std::function<void(const voxel::Region &region, ModifierType type)> &callback) {
	const bool selectFlag = (_modifierType & ModifierType::Select) == ModifierType::Select;
	if (selectFlag) {
		const math::AABB<int> a = aabb();
		Log::debug("select mode");
		select(a.mins(), a.maxs());
		if (_selectionValid) {
			callback(_selection, _modifierType);
		}
		return true;
	}

	if (volume == nullptr) {
		Log::debug("No volume given - can't perform action");
		return true;
	}

	const bool colorPickerFlag = (_modifierType & ModifierType::ColorPicker) == ModifierType::ColorPicker;
	if (colorPickerFlag) {
		const glm::ivec3 &pos = cursorPosition();
		if (volume->region().containsPoint(pos)) {
			setCursorVoxel(volume->voxel(pos));
			return true;
		}
		return false;
	}

	const bool placeFlag = (_modifierType & ModifierType::Place) == ModifierType::Place;
	const bool eraseFlag = (_modifierType & ModifierType::Erase) == ModifierType::Erase;
	const bool paintFlag = (_modifierType & ModifierType::Paint) == ModifierType::Paint;

	const bool lineFlag = (_modifierType & ModifierType::Line) == ModifierType::Line;
	if (lineFlag) {
		voxel::RawVolumeWrapper wrapper = createRawVolumeWrapper(volume);
		const glm::ivec3 &start = referencePosition();
		const glm::ivec3 &end = cursorPosition();
		voxel::Voxel voxel;
		if (eraseFlag) {
			voxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
		}
		if (placeFlag || paintFlag) {
			voxel = cursorVoxel();
		}
		voxelutil::raycastWithEndpoints(&wrapper, start, end, [=](auto &sampler) {
			const bool air = voxel::isAir(sampler.voxel().getMaterial());
			if ((!eraseFlag && !paintFlag) && !air) {
				return true;
			}
			sampler.setVoxel(voxel);
			return true;
		});
		const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
		if (modifiedRegion.isValid()) {
			callback(modifiedRegion, _modifierType);
		}
		return true;
	}
	const bool pathFlag = (_modifierType & ModifierType::Path) == ModifierType::Path;
	if (pathFlag) {
		core::List<glm::ivec3> listResult(4096);
		const glm::ivec3 &start = referencePosition();
		const glm::ivec3 &end = cursorPosition();
		voxelutil::AStarPathfinderParams<voxel::RawVolume> params(
			volume, start, end, &listResult,
			[=](const voxel::RawVolume *vol, const glm::ivec3 &pos) {
				if (lineFlag) {
					return true;
				}

				if (voxel::isBlocked(vol->voxel(pos).getMaterial())) {
					return false;
				}
				return voxelutil::isTouching(vol, pos);
			},
			4.0f, 10000, voxelutil::Connectivity::EighteenConnected);
		voxelutil::AStarPathfinder pathfinder(params);
		if (!pathfinder.execute()) {
			return false;
		}
		voxel::RawVolumeWrapper wrapper = createRawVolumeWrapper(volume);
		for (const glm::ivec3& p : listResult) {
			wrapper.setVoxel(p, cursorVoxel());
		}
		const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
		if (modifiedRegion.isValid()) {
			callback(modifiedRegion, _modifierType);
		}
		return true;
	}

	if (planeMode()) {
		voxel::RawVolumeWrapper wrapper = createRawVolumeWrapper(volume);
		voxel::Voxel hitVoxel = hitCursorVoxel();
		if (placeFlag) {
			voxelutil::extrudePlane(wrapper, cursorPosition(), cursorFace(), hitVoxel, cursorVoxel());
		} else if (eraseFlag) {
			voxelutil::erasePlane(wrapper, cursorPosition(), cursorFace(), hitVoxel);
		} else if (paintFlag) {
			voxelutil::paintPlane(wrapper, cursorPosition(), cursorFace(), hitVoxel, cursorVoxel());
		} else {
			Log::error("Unsupported plane modifier");
			return false;
		}

		const voxel::Region &modifiedRegion = wrapper.dirtyRegion();
		if (modifiedRegion.isValid()) {
			voxel::logRegion("Dirty region", modifiedRegion);
			callback(modifiedRegion, _modifierType);
		}
		return true;
	}

	ModifierVolumeWrapper wrapper(volume, _modifierType);
	const math::AABB<int> a = aabb();
	glm::ivec3 minsMirror = a.mins();
	glm::ivec3 maxsMirror = a.maxs();
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		return executeShapeAction(wrapper, a.mins(), a.maxs(), callback);
	}
	Log::debug("Execute mirror action");
	const math::AABB<int> second(minsMirror, maxsMirror);
	if (math::intersects(a, second)) {
		executeShapeAction(wrapper, a.mins(), maxsMirror, callback);
	} else {
		executeShapeAction(wrapper, a.mins(), a.maxs(), callback);
		executeShapeAction(wrapper, minsMirror, maxsMirror, callback);
	}
	_secondPosValid = false;
	_aabbSecondActionDirection = math::Axis::None;
	return true;
}

void Modifier::aabbAbort() {
	_secondPosValid = false;
	_aabbSecondActionDirection = math::Axis::None;
	_aabbMode = false;
}

bool Modifier::modifierTypeRequiresExistingVoxel() const {
	return (_modifierType & ModifierType::Erase) == ModifierType::Erase ||
		   (_modifierType & ModifierType::Paint) == ModifierType::Paint ||
		   (_modifierType & ModifierType::Select) == ModifierType::Select;
}

void Modifier::setGridResolution(int resolution) {
	_gridResolution = core_max(1, resolution);
	if (_aabbFirstPos.x % resolution != 0) {
		_aabbFirstPos.x = (_aabbFirstPos.x / resolution) * resolution;
	}
	if (_aabbFirstPos.y % resolution != 0) {
		_aabbFirstPos.y = (_aabbFirstPos.y / resolution) * resolution;
	}
	if (_aabbFirstPos.z % resolution != 0) {
		_aabbFirstPos.z = (_aabbFirstPos.z / resolution) * resolution;
	}
}

bool Modifier::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			return true;
		}
		return false;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	return true;
}

void Modifier::translate(const glm::ivec3& v) {
	_cursorPosition += v;
	_mirrorPos += v;
	if (_aabbMode) {
		_aabbFirstPos += v;
	}
}

void Modifier::setModifierType(ModifierType type) {
	if (planeMode()) {
		type |= ModifierType::Plane;
	}
	if (singleMode()) {
		type |= ModifierType::Single;
	}
	_modifierType = type;
}

void Modifier::setPlaneMode(bool state) {
	if (state) {
		_modifierType |= ModifierType::Plane;
	} else {
		_modifierType &= ~ModifierType::Plane;
	}
}

void Modifier::setSingleMode(bool state) {
	if (state) {
		_modifierType |= ModifierType::Single;
	} else {
		_modifierType &= ~ModifierType::Single;
	}
}

} // namespace voxedit
