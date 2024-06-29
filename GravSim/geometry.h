#pragma once

#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>

#include <glm/glm.hpp>

#include "structs.h"


class SphereGeometry {
public:

	std::vector<Vertex> *vertices = nullptr;
	std::vector<uint16_t>* indices = nullptr;
	std::vector<Edge> edges;
	std::vector<uint16_t> edgeIndices;
	std::map<uint16_t, uint16_t> LODoffsets;
	uint16_t mode = 0;
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
	static const uint32_t GRID_CELL_COUNT;
	glm::ivec3 GRID_DIMENSIONS = { 16, 16, 16 };
	glm::vec3 DOMAIN_DIMENSIONS = { 10, 10, 10 };


	std::vector<Particle> *particles;
	std::vector<uint32_t>* offsets;

	void createParticles(uint32_t);

};
