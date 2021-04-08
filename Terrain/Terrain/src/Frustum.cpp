#include "Frustum.h"

void Frustum::setValues(float FOV, float ar, float near, float far)
{
	fov = FOV;
	aspectR = ar;
	nearPlane = near;
	farPlane = far;

	tanHalfHFOV = std::tan(fov / 2.0f);
	tanHalfVFOV = std::tan((fov * aspectR) / 2.0f);

}

void Frustum::setCascadeEnds(float a, float b, float c, float d) // Set partitions for each cascade 
{
		// will need to update this to allow for varying number of cascades //
	cascadeEnds[0] = a;
	cascadeEnds[1] = b;
	cascadeEnds[2] = c;
	cascadeEnds[3] = d;

}

void Frustum::transformToWorldSpace()
{
	// Convert into world space. Coords in view space -> multiply by inverse of this:
	for (int i = 0; i < corners.size();	i++) 
	{
		corners.at(i) = glm::inverse(cameraView) * corners.at(i);
	}
}

void Frustum::updateCascades(int n)
{
	// clear the light view and projection
	lightViews.clear();
	lightProjection.clear();
	//
	for (int i = 0; i < n; i++)
	{
		float nearPlane = cascadeEnds[i];
		float farPlane = cascadeEnds[i + 1];
		//
		findCorners(nearPlane, farPlane); // 
		transformToWorldSpace(); // Change to world-space.
		findCentre(); // Find the centre of the cascade.
		light.setLookingAt(cascadeCentre); // Looking at the cascade's centre.
		//
		glm::vec3 p(corners[4].x, corners[4].y, corners[4].z);
		float r = glm::distance(cascadeCentre, p); // used to create the bounding box for the cascade
		lightViews.push_back(light.getLightView());
		lightProjection.push_back(light.getOrthoProjection(r)); // r, nearPlane, farPlane
	}
}

void Frustum::findCorners(float near, float far) // Find corners of frustum / fumstum split ( determined by neasr and far plane ) 
{
	corners.clear();
	float xn = near * tanHalfHFOV;
	float xf = far * tanHalfHFOV;
	float yn = near * tanHalfVFOV;
	float yf = far * tanHalfVFOV;
		// Near Face
	corners.push_back(glm::vec4(xn, yn, near, 1.0f));
	corners.push_back(glm::vec4(-xn, yn, near, 1.0f));
	corners.push_back(glm::vec4(xn, -yn, near, 1.0f));
	corners.push_back(glm::vec4(-xn, -yn, near, 1.0f));
	// Far Face
	corners.push_back(glm::vec4(xf, yf, far , 1.0f));
	corners.push_back(glm::vec4(-xf, yf, far, 1.0f));
	corners.push_back(glm::vec4(xf, -yf, far, 1.0f));
	corners.push_back(glm::vec4(-xf, -yf, far , 1.0f));
}

void Frustum::findCentre()
{
	// Sum all of the corners and return the average (which gives the centre)
	glm::vec3 sum(0.f, 0.f, 0.f);
	for (glm::vec4 v : corners)
	{
		sum += glm::vec3(v.x, v.y, v.z);
	}
	glm::vec3 average(sum.x / 8, sum.y / 8, sum.z / 8);
	cascadeCentre = average;
}

Orthographic Frustum::findMinMax()
{
	/*float minX = MAX_FLOAT;
	float maxX = MIN_FLOAT;
	float minY = MIN_FLOAT;
	float maxY = MAX_FLOAT;
	float minZ = MIN_FLOAT;
	float maxZ = MAX_FLOAT;
	*/
	return Orthographic();
}
