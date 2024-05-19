#pragma once

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>

#include <glm/glm.hpp>

#include "structs.h"


class SphereGeometry {
public:

	std::vector<Vertex> *vertices;
	std::vector<uint16_t>* indices;
	std::vector<Edge> edges;
	std::vector<uint16_t> edgeIndices;
	std::map<uint16_t, uint16_t> LODoffsets;
	uint16_t mode;
	float scale = 0.02f;
	void createSphereLongLat(uint16_t);
	void createSphereIcosphere(uint16_t);
	void createIcosahedron(bool);


private:
	
	
	glm::vec3 randomColour(uint16_t);
	void iterateIcosphere(bool);
	std::vector<uint16_t> indicesold;
	std::vector<uint16_t> indicesnew;
	void cleanGeometry(uint16_t, std::vector<Vertex>*, std::vector<uint16_t>*);
};

class ParticleGeometry {
public:
	std::vector<Particle> *particles;

	void createParticles(uint32_t);

};
