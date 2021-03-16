#include "Terrain.h"


//Terrain constructors
Terrain::Terrain(int widthIn, int heightIn, int stepSizeIn)
{
	width = widthIn;
	height = heightIn;
	stepSize = stepSizeIn;
	makeVertices(&vertices);

}

Terrain::Terrain() {
	width = 50;
	height = 50;
	stepSize = 10;
	makeVertices(&vertices);

}

unsigned int Terrain::getVAO() {
	
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, (vertices.size() * sizeof(GLfloat)), vertices.data(), GL_STATIC_DRAW);

		//xyz
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		//texture
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);


		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	
		return VAO;
}

int Terrain::getSize() {
	return vertices.size();
}

double Terrain::cycleOctaves(glm::vec3 pos, int numOctaves)
{
	//
	float total = 0.0f;
	float maxAmplitude = 0.f;
	//
	float amplitude = 100.f;
	float frequency = .005f;
	// Iterate through octaves
	for(int i = 0; i < numOctaves; i++)
	{
		double x = pos.x * frequency;
		double y = pos.y * frequency;
		//
		total = perlin.noise(x, y, 0.1) * amplitude;
		// Update values
		maxAmplitude += amplitude;
		frequency *= 2;
		amplitude /= 2;
	}
	// return result
	return (total / maxAmplitude);

}


std::vector<float> Terrain::getVertices() {
	return vertices;
}

void Terrain::makeVertices(std::vector<float> *vertices) {
	/* triangle a b c
		   b
		   | \
		   a _ c


		 triangle d f e
		   f _ e
			 \ |
			   d

		 c == d
		 b == f
		 Duplicate vertices but easier in long run! (tesselation and LOD)

		a = (x,y,z)
		b = (x, y+1)
		c = (x+1,y)

		d = (x+1,y)
		e = (x, y+1)
		f = (x+1,y+1)

		 each vertex a, b,c, etc. will have 5 data:
		 x y z u v
		  */

	for (int y = 0; y < height - 1; y++) {
		float  offSetY = y * stepSize;
		for (int x = 0; x < width - 1; x++) {
			float offSetX = x * stepSize;
			makeVertex(offSetX, offSetY, vertices);  // a
			makeVertex(offSetX, offSetY + stepSize, vertices);  // b
			makeVertex(offSetX + stepSize, offSetY, vertices);   // c
			makeVertex(offSetX + stepSize, offSetY, vertices);  //d
			makeVertex(offSetX, offSetY + stepSize, vertices);  //e
			makeVertex(offSetX + stepSize, offSetY + stepSize, vertices);  //f
		}
	}
}

void Terrain::makeVertex(int x, int y, std::vector<float> *vertices) 
{
	// Pos
	glm::vec3 pos = glm::vec3(x, y, 0.01);
	//double pn = cycleOctaves(pos,10);
	double pn = perlin.noise(x, y, 0.1);
	//x y z position
	vertices->push_back((float)x); //xPos
	vertices->push_back(pn); //yPos was 0 (id using heightmap), can now use noise function.
	vertices->push_back((float)y); //zPos

   // add texture coords
	vertices->push_back((float)x / (width*stepSize));
	vertices->push_back((float)y / (height*stepSize));


}