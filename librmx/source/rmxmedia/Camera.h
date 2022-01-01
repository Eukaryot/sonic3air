/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	Camera
*		Camera for 3D or 2D applications.
*/

#pragma once


// Axes for Camera::rotate()
enum class CameraAxis
{
	CAMERA_AXIS_X = 0,
	CAMERA_AXIS_Y,
	CAMERA_AXIS_Z,
	CAMERA_LOCAL_X,
	CAMERA_LOCAL_Y,
	CAMERA_LOCAL_Z
};


// Camera
class API_EXPORT Camera
{
public:
	Camera* mTarget;

public:
	Camera();
	Camera(const Camera& cam);
	~Camera();

	void copy(const Camera& cam);

	const Vec3f& getPosition() const  { return mPosition; }
	const Mat3f& getRotation() const  { return mRotation; }
	Vec3f getDirFront() const;
	Vec3f getDirUp() const;
	Vec3f getDirRight() const;
	Vec3f getFocusPos() const;
	void getFocusPos(Vec3f& output) const;

	void setPosition(const Vec3f& pos);
	void focusOn(const Vec3f& pos);
	void movePosition(const Vec3f& rel);
	void movePositionLocal(const Vec3f& rel);

	void setDirection(const Vec3f& front);
	void setDirection(const Vec3f& front, const Vec3f& upvector);
	void lookAt(const Vec3f& target);
	void lookAt(const Vec3f& target, const Vec3f& upvector);
	void rotate(float angle, const Vec3f& axis);
	void rotate(float angle, CameraAxis axis);

	Vec3f getEulerAngles();
	void getEulerAngles(Vec3f& angles);
	void getEulerAngles(float* angles);
	void getEulerAngles(float& alpha, float& beta, float& gamma);
	void setEulerAngles(const Vec3f& angles);
	void setEulerAngles(const float* angles);
	void setEulerAngles(float alpha, float beta, float gamma);

	bool getFocusEnabled() const	{ return mFocusEnabled; }
	float getFocusDistance() const	{ return mFocusDistance; }
	void enableFocus(bool enable);
	void setFocusDistance(float dist);
	void addFocusDistance(float rel);
	void addFocusDistanceExp(float exponent);

	float getFovX() const  { return mHalfFov[0] + mHalfFov[1]; }
	float getFovY() const  { return mHalfFov[2] + mHalfFov[3]; }
	float getHalfFov(int index);
	void setFov(float fov, float aspectratio);
	void setFovY(float fovY, float aspectratio);
	void setFov(float left, float right, float bottom, float top);

	float getOrthoData(int index);
	void setOrtho(float horiz, float vert);
	void setOrtho(float left, float right, float bottom, float top);
	void setOrtho(float left, float right, float bottom, float top, float cnear, float cfar);

	float getClipNear() const  { return mClipNear; }
	float getClipFar() const   { return mClipFar; }
	void setClipPlanes(float cnear, float cfar);

	void setCameraView() const;

	void getMatrixWorld2View(Mat4f& output) const;
	void getMatrixView2World(Mat4f& output) const;
	void getGLModelView(Mat4f& output) const;
	void getGLProjection(Mat4f& output) const;
	void getGLModelViewProj(Mat4f& output) const;

	math::Ray getRay(const Vec2f& position) const;
	math::Ray getRay(const Vec2i& pixelPosition, const Rectf& rect, bool flipY = false) const;

	void getClipPlanes(Vec4f* equations) const;

	void update(float timeElapsed, float delay);

private:
	Vec3f mPosition;
	Mat3f mRotation;
	float mFocusDistance;
	float mHalfFov[4];
	float mOrtho[4];
	float mClipNear, mClipFar;
	bool mFocusEnabled;
	bool mOrthoMode;
};
