/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


Camera::Camera()
{
	mPosition.clear();
	mRotation.setIdentity();
	mFocusDistance = 0.0f;
	mHalfFov[0] = 22.5f;
	mHalfFov[1] = 22.5f;
	mHalfFov[2] = 20.0f;
	mHalfFov[3] = 20.0f;
	mOrtho[0] = -1.0f;
	mOrtho[1] = +1.0f;
	mOrtho[2] = -1.0f;
	mOrtho[3] = +1.0f;
	mClipNear = 0.1f;
	mClipFar = 1000.0f;
	mFocusEnabled = false;
	mOrthoMode = false;
	mTarget = nullptr;
}

Camera::Camera(const Camera& cam)
{
	copy(cam);
}

Camera::~Camera()
{
}

void Camera::copy(const Camera& cam)
{
	mPosition = cam.mPosition;
	mRotation = cam.mRotation;
	mFocusDistance = cam.mFocusDistance;
	for (int i = 0; i < 4; ++i)
		mHalfFov[i] = cam.mHalfFov[i];
	for (int i = 0; i < 4; ++i)
		mOrtho[i] = cam.mOrtho[i];
	mClipNear = cam.mClipNear;
	mClipFar = cam.mClipFar;
	mFocusEnabled = cam.mFocusEnabled;
	mOrthoMode = cam.mOrthoMode;
	mTarget = cam.mTarget;
}

Vec3f Camera::getDirFront() const
{
	Vec3f output = mRotation.getRow(2);
	output.neg();
	return output;
}

Vec3f Camera::getDirUp() const
{
	return mRotation.getRow(1);
}

Vec3f Camera::getDirRight() const
{
	return mRotation.getRow(0);
}

Vec3f Camera::getFocusPos() const
{
	Vec3f output = getDirFront();
	output *= mFocusDistance;
	output += mPosition;
	return output;
}

void Camera::getFocusPos(Vec3f& output) const
{
	output = getDirFront();
	output *= mFocusDistance;
	output += mPosition;
}

void Camera::setPosition(const Vec3f& pos)
{
	mPosition = pos;
}

void Camera::focusOn(const Vec3f& pos)
{
	mPosition = getDirFront();
	mPosition *= -mFocusDistance;
	mPosition += pos;
}

void Camera::movePosition(const Vec3f& rel)
{
	mPosition.add(rel);
}

void Camera::movePositionLocal(const Vec3f& rel)
{
	mPosition += mRotation.mul(rel);
}

void Camera::setDirection(const Vec3f& front)
{
	setDirection(front, getDirUp());
}

void Camera::setDirection(const Vec3f& front, const Vec3f& upvector)
{
	Vec3f direction[3];
	direction[2] = -front;
	direction[2].normalize();
	direction[0].cross(upvector, direction[2]);
	direction[0].normalize();
	direction[1].cross(direction[2], direction[0]);
	mRotation.setRow(direction[0], 0);
	mRotation.setRow(direction[1], 1);
	mRotation.setRow(direction[2], 2);
}

void Camera::lookAt(const Vec3f& target)
{
	Vec3f tmp;
	tmp.sub(target, mPosition);
	setDirection(tmp);
}

void Camera::lookAt(const Vec3f& target, const Vec3f& upvector)
{
	Vec3f tmp;
	tmp.sub(target, mPosition);
	setDirection(tmp, upvector);
}

void Camera::rotate(float angle, const Vec3f& axis)
{
	Vec3f pos;
	if (mFocusEnabled)
		getFocusPos(pos);
	Vec3f normaxis(axis);
	normaxis.normalize();
	Mat3f rot;
	rot.setRotation3D(angle, normaxis);
	Mat3f result;
	result.mul(rot, mRotation);
	mRotation = result;
	if (mFocusEnabled)
		focusOn(pos);
}

void Camera::rotate(float angle, CameraAxis axis)
{
	Vec3f tmp;
	switch ((int)axis)
	{
		case 0:  tmp = Vec3f::UNIT_X;  break;
		case 1:  tmp = Vec3f::UNIT_X;  break;
		case 2:  tmp = Vec3f::UNIT_X;  break;
		default: tmp = mRotation.getRow((int)axis-3);  break;
	}
	rotate(angle, tmp);
}

Vec3f Camera::getEulerAngles()
{
	Vec3f angles;
	mRotation.getEulerAngles(angles);
	return angles;
}

void Camera::getEulerAngles(Vec3f& angles)
{
	mRotation.getEulerAngles(angles);
}

void Camera::getEulerAngles(float* angles)
{
	if (nullptr == angles)
		return;
	mRotation.getEulerAngles(angles[0], angles[1], angles[2]);
}

void Camera::getEulerAngles(float& alpha, float& beta, float& gamma)
{
	mRotation.getEulerAngles(alpha, beta, gamma);
}

void Camera::setEulerAngles(const Vec3f& angles)
{
	setEulerAngles(angles[0], angles[1], angles[2]);
}

void Camera::setEulerAngles(const float* angles)
{
	if (nullptr == angles)
		return;
	setEulerAngles(angles[0], angles[1], angles[2]);
}

void Camera::setEulerAngles(float alpha, float beta, float gamma)
{
	Vec3f pos;
	if (mFocusEnabled)
		getFocusPos(pos);
	mRotation.setEulerAngles(alpha, beta, gamma);
	if (mFocusEnabled)
		focusOn(pos);
}

void Camera::enableFocus(bool enable)
{
	mFocusEnabled = enable;
}

void Camera::setFocusDistance(float dist)
{
	addFocusDistance(dist - mFocusDistance);
}

void Camera::addFocusDistance(float rel)
{
	mFocusDistance += rel;
	if (mFocusEnabled)
		mPosition.add(getDirFront(), -rel);
}

void Camera::addFocusDistanceExp(float exponent)
{
	addFocusDistance(mFocusDistance * (pow(1.01f, exponent) - 1.0f));
}

float Camera::getHalfFov(int index)
{
	if (index < 0 || index >= 4)
		return 0.0f;
	return mHalfFov[index];
}

void Camera::setFov(float fov, float aspectratio)
{
	setFovY(fov / sqrt(aspectratio), aspectratio);
}

void Camera::setFovY(float fovY, float aspectratio)
{
	mOrthoMode = false;
	float hfovy = fovY / 2.0f;
	float hfovx = rad2deg(atan(tan(deg2rad(hfovy)) * aspectratio));
	mHalfFov[0] = hfovx;
	mHalfFov[1] = hfovx;
	mHalfFov[2] = hfovy;
	mHalfFov[3] = hfovy;
}

void Camera::setFov(float left, float right, float bottom, float top)
{
	mOrthoMode = false;
	mHalfFov[0] = left;
	mHalfFov[1] = right;
	mHalfFov[2] = bottom;
	mHalfFov[3] = top;
}

float Camera::getOrthoData(int index)
{
	if (index < 0 || index >= 6)
		return 0.0f;
	if (index == 4)
		return mClipNear;
	if (index == 5)
		return mClipFar;
	return mOrtho[index];
}

void Camera::setOrtho(float horiz, float vert)
{
	mOrthoMode = true;
	mOrtho[0] = -horiz / 2.0f;
	mOrtho[1] = +horiz / 2.0f;
	mOrtho[2] = -vert / 2.0f;
	mOrtho[3] = +vert / 2.0f;
}

void Camera::setOrtho(float left, float right, float bottom, float top)
{
	mOrthoMode = true;
	mOrtho[0] = left;
	mOrtho[1] = right;
	mOrtho[2] = bottom;
	mOrtho[3] = top;
}

void Camera::setOrtho(float left, float right, float bottom, float top, float cnear, float cfar)
{
	mOrthoMode = true;
	mOrtho[0] = left;
	mOrtho[1] = right;
	mOrtho[2] = bottom;
	mOrtho[3] = top;
	mClipNear = cnear;
	mClipFar = cfar;
}

void Camera::setClipPlanes(float cnear, float cfar)
{
	mClipNear = cnear;
	mClipFar = cfar;
}

void Camera::setCameraView() const
{
#ifdef ALLOW_LEGACY_OPENGL
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (mOrthoMode)
	{
		glOrtho(mOrtho[0], mOrtho[1], mOrtho[2], mOrtho[3], mClipNear, mClipFar);
	}
	else
	{
		glFrustum(-tan(deg2rad(mHalfFov[0])) * mClipNear,
				   tan(deg2rad(mHalfFov[1])) * mClipNear,
				  -tan(deg2rad(mHalfFov[2])) * mClipNear,
				   tan(deg2rad(mHalfFov[3])) * mClipNear, mClipNear, mClipFar);
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
//	glRotatef(90.0f, 1.0f, 0.0f, 0.0);
	Mat4f mv;
	getMatrixWorld2View(mv);
	mv.transpose();
	glMultMatrixf(*mv);
#else
	RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
#endif
}

void Camera::getMatrixWorld2View(Mat4f& output) const
{
	output.setIdentity();
	output[0]  = mRotation[0];
	output[1]  = mRotation[3];
	output[2]  = mRotation[6];
	output[4]  = mRotation[1];
	output[5]  = mRotation[4];
	output[6]  = mRotation[7];
	output[8]  = mRotation[2];
	output[9]  = mRotation[5];
	output[10] = mRotation[8];
	output[3]  = -(mPosition[0]*mRotation[0] + mPosition[1]*mRotation[3] + mPosition[2]*mRotation[6]);
	output[7]  = -(mPosition[0]*mRotation[1] + mPosition[1]*mRotation[4] + mPosition[2]*mRotation[7]);
	output[11] = -(mPosition[0]*mRotation[2] + mPosition[1]*mRotation[5] + mPosition[2]*mRotation[8]);
}

void Camera::getMatrixView2World(Mat4f& output) const
{
	output.setIdentity();
	output[0]  = mRotation[0];
	output[1]  = mRotation[1];
	output[2]  = mRotation[2];
	output[4]  = mRotation[3];
	output[5]  = mRotation[4];
	output[6]  = mRotation[5];
	output[8]  = mRotation[6];
	output[9]  = mRotation[7];
	output[10] = mRotation[8];
	output[3]  = mPosition[0];
	output[7]  = mPosition[1];
	output[11] = mPosition[2];
}

void Camera::getGLModelView(Mat4f& matrix) const
{
#ifdef ALLOW_LEGACY_OPENGL
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	setCameraView();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glGetFloatv(GL_MODELVIEW_MATRIX, *matrix);
	matrix.transpose();
	glPopMatrix();
#else
	RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
#endif
}

void Camera::getGLProjection(Mat4f& matrix) const
{
#ifdef ALLOW_LEGACY_OPENGL
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	setCameraView();
	glMatrixMode(GL_PROJECTION);
	glGetFloatv(GL_PROJECTION_MATRIX, *matrix);
	matrix.transpose();
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#else
	RMX_ASSERT(false, "Unsupported without legacy OpenGL support");
#endif
}

void Camera::getGLModelViewProj(Mat4f& matrix) const
{
	Mat4f matMV;
	Mat4f matP;
	getGLModelView(matMV);
	getGLProjection(matP);
	matrix.mul(matP, matMV);
}

math::Ray Camera::getRay(const Vec2f& position) const
{
	if (mOrthoMode)
	{
		Vec3f outpos = mPosition;
		outpos.add(getDirRight(), position.x * (mOrtho[1] - mOrtho[0]) + mOrtho[0]);
		outpos.add(getDirUp(),    position.y * (mOrtho[3] - mOrtho[2]) + mOrtho[2]);
		outpos.add(getDirFront(), mClipNear);

		return math::Ray(outpos, getDirFront());
	}
	else
	{
		float px = (position.x - 1.0f) * tan(deg2rad(mHalfFov[0])) + position.x * tan(deg2rad(mHalfFov[1]));
		float py = (position.y - 1.0f) * tan(deg2rad(mHalfFov[2])) + position.y * tan(deg2rad(mHalfFov[3]));

		Vec3f outdir = getDirFront();
		outdir.add(getDirRight(), px);
		outdir.add(getDirUp(), py);
		outdir.normalize();

		return math::Ray(mPosition, outdir);
	}
}

math::Ray Camera::getRay(const Vec2i& pixelPosition, const Rectf& rect, bool flipY) const
{
	Vec2f position;
	position.x = (((float)pixelPosition.x - rect.left) + 0.5f) / rect.width;
	position.y = (((float)pixelPosition.y - rect.top) + 0.5f) / rect.height;
	if (flipY)
		position.y = 1.0f - position.y;
	return getRay(position);
}

void Camera::getClipPlanes(Vec4f* equations) const
{
	static const float coeff[4][2] = { -1,0,  +1,0,  0,-1,  0,+1 };
	if (nullptr == equations)
		return;

	Vec3f vec;
	for (int i = 0; i < 4; ++i)
	{
		vec.clear();
		if (mOrthoMode)
		{
			vec.add(getDirRight(), coeff[i][0]);
			vec.add(getDirUp(),    coeff[i][1]);
		}
		else
		{
			vec.add(getDirRight(), -coeff[i][0]);
			vec.add(getDirUp(),    -coeff[i][1]);
			vec.add(getDirFront(), tan(deg2rad(mHalfFov[i])));
		}
		vec.normalize();
		vec.copyTo(*equations[i]);
		equations[i][3] = -vec.dot(mPosition);
	}
	vec = getDirFront();
	vec.copyTo(*equations[4]);
	equations[4][3] = -vec.dot(mPosition) - mClipNear;
	vec.neg();
	vec.copyTo(*equations[5]);
	equations[5][3] = -vec.dot(mPosition) + mClipFar;
}

void Camera::update(float timeElapsed, float delay)
{
	if (nullptr == mTarget)
		return;
	float factor = saturate(1.0f - powf(delay, timeElapsed));
	mPosition.interpolate(mTarget->mPosition, factor);
	Vec3f direction[2];
	direction[0].interpolate(getDirRight(), mTarget->getDirRight(), factor);
	direction[1].interpolate(getDirUp(), mTarget->getDirUp(), factor);
	mRotation.setOrthonormal(direction[0], direction[1]);
	mFocusDistance += (mTarget->mFocusDistance - mFocusDistance) * factor;
}
