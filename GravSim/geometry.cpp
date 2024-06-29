#include <cstdlib>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>



#include "structs.h"
#include "geometry.h"

void SphereGeometry::createSphereLongLat(uint16_t LOD) {

	uint16_t sectors = glm::pow(2, 3 * LOD);
	uint16_t stacks = sectors / 2;



	std::cout << sectors << " | " << stacks << std::endl;

	double sectorStep = glm::two_pi<double>() / sectors;
	double stackStep = glm::pi<double>() / stacks;

	vertices->clear();
	indices->clear();


	Vertex north;
	north.pos = glm::vec3(0, 1, 0);
	north.colour = randomColour(mode);
	north.texCoord = glm::vec2(0);
	north.pos *= scale;

	Vertex south;
	south.pos = glm::vec3(0, -1, 0);
	south.colour = randomColour(mode);
	south.texCoord = glm::vec2(0);
	south.pos *= scale;

	vertices->push_back(north);
	vertices->push_back(south);
	

	for (int i = 0; i < sectors; i++) {
		double sectorAngle = i * sectorStep;
		for (int j = 1; j < stacks; j++) {
			double stackAngle = -glm::half_pi<double>() + j * stackStep;
			Vertex newVertex;
			//std::cout << j << " | ";
			
			newVertex.pos.x = glm::sin(sectorAngle) * glm::sqrt(1 - glm::pow(glm::sin(stackAngle), 2));
			newVertex.pos.y = glm::sin(stackAngle);
			newVertex.pos.z = glm::cos(sectorAngle) * glm::sqrt(1 - glm::pow(glm::sin(stackAngle), 2));
			newVertex.colour = randomColour(mode);
			newVertex.texCoord = glm::vec2(0);
			newVertex.pos *= scale;

			vertices->push_back(newVertex);

			//generate indices anticlockwise
			uint16_t index1, index2, index3;

			if (j == 1) { //special case if j=1, 3 triangles needs to be drawn at south pole
				index1 = 1; //south
				index2 = ((i +1)%sectors)* (stacks - 1) + j +1; // one sector over and up one
				index3 = i * (stacks - 1) + j +1;


				indices->push_back(index1);
				indices->push_back(index2);
				indices->push_back(index3);
				//std::cout << index1 << " | " << index2 << " | " << index3 << " | ";

				index1 = i * (stacks - 1) + 1 + j;
				index2 = ((i + 1) % sectors) * (stacks - 1) + 1 + j + 1;
				index3 = i * (stacks - 1) + 1 + j + 1;

				indices->push_back(index1);
				indices->push_back(index2);
				indices->push_back(index3);
				//std::cout << index1 << " | " << index2 << " | " << index3 << " | ";

				index1 = i * (stacks - 1) + 1 + j;
				index2 = ((i + 1) % sectors) * (stacks - 1) + 1 + j;
				index3 = ((i + 1) % sectors) * (stacks - 1) + 1 + j + 1;

				indices->push_back(index1);
				indices->push_back(index2);
				indices->push_back(index3);
				//std::cout << index1 << " | " << index2 << " | " << index3 << std::endl;
			}
			else if (j==stacks-1){ // special case if at north pole, 1 traingle with this,1 sector to right , north
				index1 = i * (stacks - 1) + j + 1;
				index2 = ((i + 1) % sectors) * (stacks - 1) + j + 1;
				index3 = 0;

				indices->push_back(index1);
				indices->push_back(index2);
				indices->push_back(index3);
				//std::cout << index1 << " | " << index2 << " | " << index3 << std::endl;
			}
			else {
				index1 = i * (stacks - 1) + 1 + j;
				index2 = ((i + 1) % sectors) * (stacks - 1) + 1 + j + 1;
				index3 = i * (stacks - 1) + 1 + j + 1;

				indices->push_back(index1);
				indices->push_back(index2);
				indices->push_back(index3);
				//std::cout << index1 << " | " << index2 << " | " << index3 << " | ";

				index1 = i * (stacks - 1) + 1 + j;
				index2 = ((i + 1) % sectors) * (stacks - 1) + 1 + j;
				index3 = ((i + 1) % sectors) * (stacks - 1) + 1 + j + 1;

				indices->push_back(index1);
				indices->push_back(index2);
				indices->push_back(index3);
				//std::cout << index1 << " | " << index2 << " | " << index3 << std::endl;
				
				if (index1 > UINT16_MAX || index2 > UINT16_MAX || index3 > UINT16_MAX) {
					throw std::runtime_error("LOD to high!");
				}


			}


		}


		
	}

	Vertex ground1;
	Vertex ground2;
	Vertex ground3;
	Vertex ground4;
	uint16_t scale = 20;
	uint16_t height = 5;

	ground1.pos = glm::vec3(1, 1, -1);
	ground1.colour = glm::vec3(0.2, 0.8, 0.5);
	ground1.texCoord = glm::vec3(0);
	ground2.pos = glm::vec3(-1, 1, -1);
	ground2.colour = glm::vec3(0.2, 0.8, 0.4);
	ground2.texCoord = glm::vec3(0);
	ground3.pos = glm::vec3(-1, -1, -1);
	ground3.colour = glm::vec3(0.2, 0.9, 0.5);
	ground3.texCoord = glm::vec3(0);
	ground4.pos = glm::vec3(1, -1, -1);
	ground4.colour = glm::vec3(0.3, 0.8, 0.5);
	ground4.texCoord = glm::vec3(0);


	ground1.pos.x *= scale;
	ground1.pos.y *= scale;
	ground1.pos.z *= height;
	ground2.pos.x *= scale;
	ground2.pos.y *= scale;
	ground2.pos.z *= height;
	ground3.pos.x *= scale;
	ground3.pos.y *= scale;
	ground3.pos.z *= height;
	ground4.pos.x *= scale;
	ground4.pos.y *= scale;
	ground4.pos.z *= height;






	uint16_t indexStart = vertices->size();
	vertices->push_back(ground1);
	vertices->push_back(ground2);
	vertices->push_back(ground3);
	vertices->push_back(ground4);


	//std::cout << indexStart << std::endl;
	indices->push_back(indexStart);
	indices->push_back(indexStart + 1);
	indices->push_back(indexStart + 2);
	indices->push_back(indexStart);
	indices->push_back(indexStart + 2);
	indices->push_back(indexStart + 3);
	//std::cout << (*indices)[indices->size() - 2] << std::endl;
	//std::cout << indices->size() / 3 << " triangles" << std::endl;
	//int count = 0;

	//for (auto index : *indices) {
	//	std::cout << (*vertices)[index].pos.x << ", " << (*vertices)[index].pos.y << ", " << (*vertices)[index].pos.z << " | ";
	//	if ((count - 2) % 3) {
	//		std::cout << std::endl;
	//	}
	//	count++;
	//}
}
void SphereGeometry::createSphereIcosphere(uint16_t LOD) {
	createIcosahedron(true);
	//indicesold = *indices;
	for (uint16_t iter = 0; iter < LOD; iter++) {
		iterateIcosphere(true);
	}
	//*indices = indicesold;
}
void SphereGeometry::createIcosahedron(bool edgeToggle) {
	vertices->clear();
	indices->clear();
	Vertex south{};
	south.pos = glm::vec3(0, 0, -1) * scale;
	vertices->push_back(south);
	if (!edgeToggle) {
		for (uint16_t i = 0; i < 5; i++) {
			//do the bottom layer
			Vertex newVertex{};
			newVertex.pos = glm::vec3(glm::cos(glm::two_pi<float>() * i / 5), glm::sin(glm::two_pi<float>() * i / 5), -glm::sin(glm::atan(0.5))) * scale;
			newVertex.colour = randomColour(mode);
			vertices->push_back(newVertex);
			uint16_t thisindex = i + 1;
			uint16_t nextindex = ((i + 1) % 5) + 1;
			indices->push_back(0);
			indices->push_back(thisindex);
			indices->push_back(nextindex);
		}
		for (uint16_t i = 0; i < 5; i++) {
			Vertex newVertex{};
			//now the middle layer
			newVertex.pos = glm::vec3(glm::cos(glm::two_pi<float>() * (i - 0.5) / 5), glm::sin(glm::two_pi<float>() * (i - 0.5) / 5), glm::sin(glm::atan(0.5))) * scale;
			newVertex.colour = randomColour(mode);
			vertices->push_back(newVertex);
			//newVertex.pos *= 20;
			//middle layer indices
			//1, 6, 7 then 1, 7, 2 etc // i-5
			uint16_t thisindex = i + 6;
			uint16_t nextindex = ((i + 1) % 5) + 6;
			uint16_t thisindexdown = i + 1;
			uint16_t nextindexdown = ((i + 1) % 5) + 1;

			indices->push_back(thisindexdown);
			indices->push_back(thisindex);
			indices->push_back(nextindex);

			indices->push_back(thisindexdown);
			indices->push_back(nextindex);
			indices->push_back(nextindexdown);
			//now the top
			indices->push_back(thisindex);
			indices->push_back(11);
			indices->push_back(nextindex);
		}
		Vertex north{};
		north.pos = glm::vec3(0, 0, 1) * scale;
		vertices->push_back(north);
	}
	else {
		edges.clear();
		edgeIndices.clear();
		for (uint16_t i = 0; i < 5; i++) {
			//do the bottom layer
			Vertex newVertex{};
			newVertex.pos = glm::vec3(glm::cos(glm::two_pi<float>() * i / 5), glm::sin(glm::two_pi<float>() * i / 5), -glm::sin(glm::atan(0.5))) * scale;
			newVertex.colour = randomColour(mode);
			vertices->push_back(newVertex);
			Edge newEdge{};
			newEdge.vert0 = 0;
			newEdge.vert1 = i+1;
			edges.push_back(newEdge);

			uint16_t edgeIndex0 = i; // 0
			uint16_t edgeIndex1 = i + 5; //5
			uint16_t edgeIndex2 = (i + 1) % 5; //1

			edgeIndices.push_back(edgeIndex0);
			edgeIndices.push_back(edgeIndex1);
			edgeIndices.push_back(edgeIndex2);
		}
		for (uint16_t i = 0; i < 5; i++) {
			Edge newEdge{};
			newEdge.vert0 = i+1; //1
			newEdge.vert1 = (i + 1) % 5 + 1; //2
			edges.push_back(newEdge);
		}
		for (uint16_t i = 0; i < 5; i++) {
			Vertex newVertex{};
			//now the middle layer
			newVertex.pos = glm::vec3(glm::cos(glm::two_pi<float>() * (i - 0.5) / 5), glm::sin(glm::two_pi<float>() * (i - 0.5) / 5), glm::sin(glm::atan(0.5))) * scale;
			newVertex.colour = randomColour(mode);
			vertices->push_back(newVertex);
			Edge newEdge{};
			newEdge.vert0 = i + 1; //1 for example
			newEdge.vert1 = i + 5 + 1; //6 : the left hand diagonals;
			edges.push_back(newEdge);
		}
		for (uint16_t i = 0; i < 5; i++) {
			Edge newEdge{};
			newEdge.vert0 = i + 1; //1
			newEdge.vert1 = (i + 1) % 5 + 5 + 1; //7 right hand diagonals
			edges.push_back(newEdge);
		}
		for (uint16_t i = 0; i < 5; i++) {
			Edge newEdge{};
			newEdge.vert0 = i + 1 + 5; //6
			newEdge.vert1 = (i + 1) % 5 + 5 + 1; //7
			edges.push_back(newEdge);


			//example would be (10, 20, 15) then (15, 11, 5)
			uint16_t edgeIndex0 = (i + 10);
			uint16_t edgeIndex1 = (i + 20);
			uint16_t edgeIndex2 = (i + 15);

			edgeIndices.push_back(edgeIndex0);
			edgeIndices.push_back(edgeIndex1);
			edgeIndices.push_back(edgeIndex2);

			edgeIndex0 = (i + 15);
			edgeIndex1 = (i + 1) % 5 + 10;
			edgeIndex2 = (i + 5);

			edgeIndices.push_back(edgeIndex0);
			edgeIndices.push_back(edgeIndex1);
			edgeIndices.push_back(edgeIndex2);
		}
		for (uint16_t i = 0; i < 5; i++) {
			Edge newEdge{};
			newEdge.vert0 = i + 6;
			newEdge.vert1 = 11;
			edges.push_back(newEdge);
			//final triangles so 25, 26, 20;
			uint16_t edgeIndex0 = (i + 25);
			uint16_t edgeIndex1 = (i + 1) % 5 + 25;
			uint16_t edgeIndex2 = (i + 20);

			edgeIndices.push_back(edgeIndex0);
			edgeIndices.push_back(edgeIndex1);
			edgeIndices.push_back(edgeIndex2);
		} 
		Vertex north{};
		north.pos = glm::vec3(0, 0, 1) * scale;
		vertices->push_back(north);
		//then convert edge indices to triangles;
		//std::cout << edges.size();

		//for (uint16_t i = 0; i < edges.size(); i++) {
		//	std::cout << "Edge: " << i << " | " << edges[i].vert0 << " " << edges[i].vert1 << std::endl;;
		//}

		for (uint16_t triangleIndex = 0; triangleIndex < edgeIndices.size() / 3; triangleIndex++) {
			
			uint16_t vert0 = edges[edgeIndices[3*triangleIndex]].vert0;
			uint16_t vert1 = edges[edgeIndices[3*triangleIndex]].vert1;
			uint16_t vert2 = edges[edgeIndices[3*triangleIndex+1]].vert0;
			uint16_t vert3 = edges[edgeIndices[3*triangleIndex+1]].vert1;
			uint16_t vert4 = edges[edgeIndices[3*triangleIndex+2]].vert0;
			uint16_t vert5 = edges[edgeIndices[3*triangleIndex+2]].vert1;

			
			//std::cout << "Index: " << triangleIndex << " | " << vert0 << " " << vert1 << " " << vert2 << " " << vert3 << " " << vert4 << " " << vert5 << " Edges: " << edgeIndices[3 * triangleIndex] << " " << edgeIndices[3 * triangleIndex + 1] << " " << edgeIndices[3 * triangleIndex + 2] << " " << std::endl;

			uint16_t outVert0 = 0;
			uint16_t outVert1 = 0;
			uint16_t outVert2 = 0;
		
			if (vert1 == vert2) {
				//std::cout << "Case 1: ";
				outVert0 = vert0;
				outVert1 = vert1;
				outVert2 = vert3;
			}
			else if (vert1 == vert3) {
				//std::cout << "Case 2: ";
				outVert0 = vert0;
				outVert1 = vert1;
				outVert2 = vert2;
			}
			else if (vert0 == vert2) {
				//std::cout << "Case 3: ";
				outVert0 = vert1;
				outVert1 = vert0;
				outVert2 = vert3;
			}
			else if (vert0 == vert3) {
				//std::cout << "Case 4: ";
				outVert0 = vert1;
				outVert1 = vert0;
				outVert2 = vert2;
			}
			else { throw std::runtime_error(("No Common Vertices on edges!")); }

			//std::cout << outVert0 << " " << outVert1 << " " << outVert2 << std::endl;

			indices->push_back(outVert0);
			indices->push_back(outVert1);
			indices->push_back(outVert2);

		}

		for (uint16_t i = 0; i < vertices->size(); i++) {
			(*vertices)[i].pos = glm::normalize((*vertices)[i].pos) * scale;
		}
	}
}




void SphereGeometry::iterateIcosphere(bool edgeToggle) {

	auto start = std::chrono::high_resolution_clock::now();


	if (!edgeToggle) {
		//this algorithm is inefficient and creates 2 x the required amount of vertices! due to the fact that each edge (which spawns a vertex) is shared by two triangles.
		uint16_t indexoffset = 0;
		uint16_t safeValue = vertices->size(); //we know that starting list is free of duplicates
		for (uint16_t index = 0; index < indicesold.size(); index += 3) {
			indexoffset = vertices->size();
			std::array<Vertex, 3>rootVertices = { (*vertices)[indicesold[index]], (*vertices)[indicesold[index + 1]], (*vertices)[indicesold[index + 2]] };
			std::array<Vertex, 3>newVertices{};
			newVertices[0].pos = glm::normalize(rootVertices[0].pos + rootVertices[1].pos) * scale;
			newVertices[1].pos = glm::normalize(rootVertices[1].pos + rootVertices[2].pos) * scale;
			newVertices[2].pos = glm::normalize(rootVertices[2].pos + rootVertices[0].pos) * scale;
			newVertices[0].colour = randomColour(mode);
			newVertices[1].colour = randomColour(mode);
			newVertices[2].colour = randomColour(mode);

			newVertices[0].colour = (rootVertices[0].colour + rootVertices[1].colour) * 0.5f;
			newVertices[1].colour = (rootVertices[1].colour + rootVertices[2].colour) * 0.5f;
			newVertices[2].colour = (rootVertices[2].colour + rootVertices[0].colour) * 0.5f;

			vertices->push_back(newVertices[0]);
			vertices->push_back(newVertices[1]);
			vertices->push_back(newVertices[2]);

			//bottom left
			indicesnew.push_back(indicesold[index]);
			indicesnew.push_back(indexoffset);
			indicesnew.push_back(indexoffset + 2);
			//top
			indicesnew.push_back(indexoffset);
			indicesnew.push_back(indicesold[index + 1]);
			indicesnew.push_back(indexoffset + 1);
			//bottom right
			indicesnew.push_back(indexoffset + 2);
			indicesnew.push_back(indexoffset + 1);
			indicesnew.push_back(indicesold[index + 2]);
			//centre
			indicesnew.push_back(indexoffset);
			indicesnew.push_back(indexoffset + 1);
			indicesnew.push_back(indexoffset + 2);
		}
		cleanGeometry(safeValue, vertices, &indicesnew);

		indicesold = indicesnew;
		indicesnew.clear();
	}
	else {
		//step 1 is create all the new edges & vertices by halving every edge
		std::vector<Vertex> workingVertices;
		std::vector<Edge> workingEdges;
		std::vector<uint16_t>workingIndices;
		std::vector<uint16_t>workingEdgeIndices;

		workingVertices = *vertices;
		uint16_t startVertexCount = vertices->size();

		uint16_t endVertexCount = (uint16_t)vertices->size() + ((uint16_t)indices->size() >> 1);
		uint16_t endIndexCount = (uint16_t)indices->size() * 4;
		//std::cout << endVertexCount << " " << endIndexCount << std::endl;
		workingVertices.reserve(endVertexCount);
		workingIndices.reserve(endIndexCount);
		workingEdgeIndices.reserve(endIndexCount);
		workingEdges.reserve(endIndexCount / 2);

		for (uint16_t edgeIndex = 0; edgeIndex < edges.size(); edgeIndex++) {
			Vertex vert0 = (*vertices)[edges[edgeIndex].vert0];
			Vertex vert2 = (*vertices)[edges[edgeIndex].vert1];
			Vertex vert1{};
			vert1.pos = glm::normalize(vert0.pos + vert2.pos) * scale;
			vert1.colour = vert0.colour + vert2.colour;
			vert1.colour *= 0.5;
			//vert1.colour = randomColour(mode);

			workingVertices.push_back(vert1);

			Edge edge1{};
			Edge edge2{};

			edge1.vert0 = edges[edgeIndex].vert0;
			edge1.vert1 = startVertexCount + edgeIndex;
			edge2.vert0 = startVertexCount + edgeIndex;
			edge2.vert1 = edges[edgeIndex].vert1;

			workingEdges.push_back(edge1);
			workingEdges.push_back(edge2);
		}
		// step 2 is for every triangle, use the old edge indexes to find the new edges, vertices & create the new triangles;
		for (uint16_t triangleIndex = 0; triangleIndex < edgeIndices.size(); triangleIndex += 3) {
			uint16_t edgeIndex0 = edgeIndices[triangleIndex];
			uint16_t edgeIndex1 = edgeIndices[triangleIndex + 1];
			uint16_t edgeIndex2 = edgeIndices[triangleIndex + 2];

			uint16_t vert0 = edges[edgeIndices[triangleIndex]].vert0;
			uint16_t vert1 = edges[edgeIndices[triangleIndex]].vert1;
			uint16_t vert2 = edges[edgeIndices[triangleIndex + 1]].vert0;
			uint16_t vert3 = edges[edgeIndices[triangleIndex + 1]].vert1;
			uint16_t vert4 = edges[edgeIndices[triangleIndex + 2]].vert0;
			uint16_t vert5 = edges[edgeIndices[triangleIndex + 2]].vert1;

			bool edgeInvert0;
			bool edgeInvert1;
			bool edgeInvert2;

			if (vert1 == vert2 && vert3 == vert4) {
				//std::cout << "Case 1";
				edgeInvert0 = false;
				edgeInvert1 = false;
				edgeInvert2 = false;
			}
			else if (vert0 == vert2 && vert3 == vert4) {
				//std::cout << "Case 2";
				edgeInvert0 = true;
				edgeInvert1 = false;
				edgeInvert2 = false;
			}
			else if (vert1 == vert3 && vert2 == vert4) {
				//std::cout << "Case 3";
				edgeInvert0 = false;
				edgeInvert1 = true;
				edgeInvert2 = false;
			}
			else if (vert1 == vert2 && vert3 == vert5) {
				//std::cout << "Case 4";
				edgeInvert0 = false;
				edgeInvert1 = false;
				edgeInvert2 = true;
			}
			else if (vert1 == vert3 && vert2 == vert5) {
				//std::cout << "Case 5";
				edgeInvert0 = false;
				edgeInvert1 = true;
				edgeInvert2 = true;
			}
			else if (vert0 == vert2 && vert3 == vert5) {
				//std::cout << "Case 6";
				edgeInvert0 = true;
				edgeInvert1 = false;
				edgeInvert2 = true;
			}
			else if (vert0 == vert3 && vert2 == vert4) {
				//std::cout << "Case 7";
				edgeInvert0 = true;
				edgeInvert1 = true;
				edgeInvert2 = false;
			}
			else if (vert0 == vert3 && vert2 == vert5) {
				//std::cout << "Case 8";
				edgeInvert0 = true;
				edgeInvert1 = true;
				edgeInvert2 = true;
			}
			else { throw std::runtime_error("Failed to find case for edge inversion!"); }


			// looking at upsideown triangle of original vertices 0, 1, 2, and edges 0, 5, 1;
			//start with making 3 new edges, starting with top left and going ccw
			Edge newEdge0{};
			Edge newEdge1{};
			Edge newEdge2{};

			newEdge0.vert0 = workingEdges[2 * edgeIndex0].vert1;
			newEdge0.vert1 = workingEdges[2 * edgeIndex1].vert1;
			newEdge1.vert0 = workingEdges[2 * edgeIndex1].vert1;
			newEdge1.vert1 = workingEdges[2 * edgeIndex2].vert1;
			newEdge2.vert0 = workingEdges[2 * edgeIndex2].vert1;
			newEdge2.vert1 = workingEdges[2 * edgeIndex0].vert1;

			uint16_t newEdgeIndex0 = workingEdges.size();
			uint16_t newEdgeIndex1 = workingEdges.size() + 1;
			uint16_t newEdgeIndex2 = workingEdges.size() + 2;

			//std::cout << " " << newEdge0.vert0 << " " << newEdge0.vert1 << " " << newEdge1.vert0 << " " << newEdge1.vert1 << " " << newEdge2.vert0 << " " << newEdge2.vert1 << " | ";

			workingEdges.push_back(newEdge0);
			workingEdges.push_back(newEdge1);
			workingEdges.push_back(newEdge2);

			//start with top left tri;

			uint16_t newIndex0;
			uint16_t newIndex1;
			uint16_t newIndex2;

			//std::cout << (2 * edgeIndex0 + 1) << " " << (2 * edgeIndex1) << " " << (newEdgeIndex0) << std::endl;
			//std::cout << workingEdges[(2 * edgeIndex0 + 1)].vert0 << " " << workingEdges[(2 * edgeIndex0 + 1)].vert1 << " " << workingEdges[(2 * edgeIndex1)].vert0 << " " << workingEdges[(2 * edgeIndex1)].vert1 << " " << workingEdges[newEdgeIndex0].vert0 << " " << " " << workingEdges[newEdgeIndex0].vert1 << std::endl;

			//top left
			if (edgeInvert0) { newIndex0 = 2 * edgeIndex0; }
			else { newIndex0 = 2 * edgeIndex0 + 1; }
			if (edgeInvert1) { newIndex1 = 2 * edgeIndex1 + 1; }
			else { newIndex1 = 2 * edgeIndex1; }
			newIndex2 = newEdgeIndex0;

			workingEdgeIndices.push_back(newIndex0);
			workingEdgeIndices.push_back(newIndex1);
			workingEdgeIndices.push_back(newIndex2);

			//top right
			newIndex0 = newEdgeIndex1;
			if (edgeInvert1) { newIndex1 = 2 * edgeIndex1; }
			else { newIndex1 = 2 * edgeIndex1 + 1; }
			if (edgeInvert2) { newIndex2 = 2 * edgeIndex2 + 1; }
			else { newIndex2 = 2 * edgeIndex2; }
			
			workingEdgeIndices.push_back(newIndex0);
			workingEdgeIndices.push_back(newIndex1);
			workingEdgeIndices.push_back(newIndex2);

			//bottom
			if (edgeInvert0) { newIndex0 = 2 * edgeIndex0 + 1; }
			else { newIndex0 = 2 * edgeIndex0; }
			newIndex1 = newEdgeIndex2;
			if (edgeInvert2) { newIndex2 = 2 * edgeIndex2; }
			else { newIndex2 = 2 * edgeIndex2 + 1; }

			workingEdgeIndices.push_back(newIndex0);
			workingEdgeIndices.push_back(newIndex1);
			workingEdgeIndices.push_back(newIndex2);

			//centre
			newIndex0 = newEdgeIndex0;
			newIndex1 = newEdgeIndex1;
			newIndex2 = newEdgeIndex2;

			workingEdgeIndices.push_back(newIndex0);
			workingEdgeIndices.push_back(newIndex1);
			workingEdgeIndices.push_back(newIndex2);


			//workingEdgeIndices.push_back(2 * edgeIndex0 + 1);
			//workingEdgeIndices.push_back(2 * edgeIndex1);
			//workingEdgeIndices.push_back(newEdgeIndex0);

			////now top right
			//workingEdgeIndices.push_back(2 * edgeIndex1 + 1);
			//workingEdgeIndices.push_back(2 * edgeIndex2);
			//workingEdgeIndices.push_back(newEdgeIndex1);
			// 
			////now bottom
			//workingEdgeIndices.push_back(2 * edgeIndex2 + 1);
			//workingEdgeIndices.push_back(2 * edgeIndex0);
			//workingEdgeIndices.push_back(newEdgeIndex2);

			////now centre
			//workingEdgeIndices.push_back(newEdgeIndex0);
			//workingEdgeIndices.push_back(newEdgeIndex1);
			//workingEdgeIndices.push_back(newEdgeIndex2);
		}
		//all that is left is to finalise everything and then convert edge indices to indices;
		//std::cout << "Edges: " << workingEdges.size() << " Edge Indices: " << workingEdgeIndices.size() << std::endl;



		edgeIndices = workingEdgeIndices;
		edges = workingEdges;
		(*vertices) = workingVertices;
		//then convert edge indices to triangles;
		//std::cout <<"Edges: "<< edges.size()<<" Indices";

		//for (uint16_t i = 0; i < edges.size(); i++) {
		//	std::cout << "Edge: " << i << " | " << edges[i].vert0 << " " << edges[i].vert1 << std::endl;;
		//}
		indices->clear();
		indices->reserve(endIndexCount);

		for (uint16_t triangleIndex = 0; triangleIndex < edgeIndices.size() / 3; triangleIndex++) {

			uint16_t vert0 = edges[edgeIndices[3 * triangleIndex]].vert0;
			uint16_t vert1 = edges[edgeIndices[3 * triangleIndex]].vert1;
			uint16_t vert2 = edges[edgeIndices[3 * triangleIndex + 1]].vert0;
			uint16_t vert3 = edges[edgeIndices[3 * triangleIndex + 1]].vert1;
			uint16_t vert4 = edges[edgeIndices[3 * triangleIndex + 2]].vert0;
			uint16_t vert5 = edges[edgeIndices[3 * triangleIndex + 2]].vert1;


			//std::cout << "Index: " << triangleIndex << " | " << vert0 << " " << vert1 << " " << vert2 << " " << vert3 << " " << vert4 << " " << vert5 << " Edges: " << edgeIndices[3 * triangleIndex] << " " << edgeIndices[3 * triangleIndex + 1] << " " << edgeIndices[3 * triangleIndex + 2] << " " << std::endl;

			uint16_t outVert0 = 0;
			uint16_t outVert1 = 0;
			uint16_t outVert2 = 0;

			if (vert1 == vert2) {
				//std::cout << "Case 1: ";
				outVert0 = vert0;
				outVert1 = vert1;
				outVert2 = vert3;
			}
			else if (vert1 == vert3) {
				//std::cout << "Case 2: ";
				outVert0 = vert0;
				outVert1 = vert1;
				outVert2 = vert2;
			}
			else if (vert0 == vert2) {
				//std::cout << "Case 3: ";
				outVert0 = vert1;
				outVert1 = vert0;
				outVert2 = vert3;
			}
			else if (vert0 == vert3) {
				//std::cout << "Case 4: ";
				outVert0 = vert1;
				outVert1 = vert0;
				outVert2 = vert2;
			}
			else { throw std::runtime_error(("No Common Vertices on edges!")); }

			//std::cout << outVert0 << " " << outVert1 << " " << outVert2 << std::endl;

			indices->push_back(outVert0);
			indices->push_back(outVert1);
			indices->push_back(outVert2);
		}
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double, std::milli> duration = end - start;

	std::cout << "This iteration took: " << duration << " to make "<<vertices->size()<<" vertices with "<<indices->size() <<" indices" << std::endl;
}

void SphereGeometry::cleanGeometry(uint16_t safeValue, std::vector<Vertex>* cleanVertices, std::vector<uint16_t>* cleanIndices) { // does not work
	std::cout << "Unclean: Vertices: " << cleanVertices->size() << " Indices: " << cleanIndices->size() << std::endl;
	for (uint16_t iterator = safeValue; iterator < cleanVertices->size(); iterator++) {
		glm::vec3 searchKey = (*vertices)[iterator].pos;
		for (uint16_t cursor = safeValue; cursor < cleanVertices->size(); cursor++) {
			if ((*vertices)[cursor].pos == searchKey && cursor!=iterator) {
				for (uint16_t indexCursor = 0; indexCursor < cleanIndices->size(); indexCursor++) {
					if ((*cleanIndices)[indexCursor] == cursor) {
						(*cleanIndices)[indexCursor] = iterator;
						std::cout << "Changing index at "<<indexCursor<<" from " << cursor << " to " << iterator << std::endl;
						//break;
					}
				}
				vertices->erase(vertices->begin() + cursor);
				cursor--;
				break;
			}
		}
	}
	std::cout << "Cleaned: Vertices: " << cleanVertices->size() << " Indices: " << cleanIndices->size() << std::endl;
}


glm::vec3 SphereGeometry::randomColour(uint16_t mode) {
	if (mode == 0) {
		return glm::vec3({ glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1) });
	}
	else if (mode == 1) {
		return glm::vec3({ 0, glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1) });
	}
	else if (mode == 2) {
		return glm::vec3({ glm::linearRand<float>(0, 1),0, glm::linearRand<float>(0, 1) });
	}
	else if (mode == 3) {
		return glm::vec3({glm::linearRand<float>(0, 1), glm::linearRand<float>(0, 1),0 });
	}
	else if (mode == 4) {
		return glm::vec3({ 0, 0, glm::linearRand<float>(0, 1) });
	}
	else if (mode == 5) {
		return glm::vec3({ 0, glm::linearRand<float>(0, 1) ,0});
	}
	else if (mode == 6) {
		return glm::vec3({ glm::linearRand<float>(0, 1) , 0, 0});
	}
}

void ParticleGeometry::createParticles(uint32_t size) {
	std::srand(glfwGetTime());

	particles->reserve(size);

	for (size_t i = 0; i < size; i++) {
		Particle part{};
		glm::vec3 up = { 0.0f, 0.0f, 1.0f };
		part.position = { glm::gaussRand<float>(5.0f, 1.3f), glm::gaussRand<float>(5.0f, 1.3f), glm::gaussRand<float>(5.0f, 1.3f) };
		part.velocity = { glm::gaussRand<float>(0.0f, 0.3f), glm::gaussRand<float>(0.0f, 0.3f), glm::gaussRand<float>(0.0f, 0.3f) };

		//part.position.z *= 0.1f;
		//part.position.x *= 0.6f;
		//part.position.y *= 0.6f;
		part.velocity = 0.2f * glm::cross(part.position, up);
		part.velocity = glm::normalize(part.velocity);
		part.velocity /= glm::pow(glm::dot(part.position, part.position), 0.25f);
		float pow = glm::linearRand<float>(-1, 1);
		part.mass = glm::pow(2.0f, pow);
		//part.mass /= glm::pow(glm::dot(part.position, part.position), 0.5);
		//part.mass = glm::clamp(part.mass, 0.1, 1.0);
		
		//part.mass = 100;
		// 
		float angle = (glm::two_pi<float>() / size) * i;

		//part.position = { 10*glm::sin(angle), 10*glm::cos(angle), 0 };
		
		//part.velocity = { 1,1,1 };
		//part.velocity = { 0,0,0 };
		//std::cout << "Position: " << part.position.x << " | " << part.position.y << " | " << part.position.z << " | " << glm::sqrt(glm::dot(part.position, part.position)) << std::endl;
		//std::cout << "Velocity: " << part.velocity.x << " | " << part.velocity.y << " | " << part.velocity.z << " | " << glm::sqrt(glm::dot(part.velocity, part.velocity)) << std::endl;
		if (part.position.x < 0 || part.position.y < 0 || part.position.z < 0 || part.position.x >= DOMAIN_DIMENSIONS.x || part.position.y >= DOMAIN_DIMENSIONS.y || part.position.z >= DOMAIN_DIMENSIONS.z) {
			part.position = { 1, 1, 1 };
		}

		part.newIndex = 0;
		glm::ivec3 cellPos;
		cellPos.x = floor(part.position.x * GRID_DIMENSIONS.x / DOMAIN_DIMENSIONS.x);
		cellPos.y = floor(part.position.y * GRID_DIMENSIONS.y / DOMAIN_DIMENSIONS.y);
		cellPos.z = floor(part.position.z * GRID_DIMENSIONS.z / DOMAIN_DIMENSIONS.z);
		part.cell = cellPos.x + GRID_DIMENSIONS.x * cellPos.y + GRID_DIMENSIONS.x * GRID_DIMENSIONS.y * cellPos.z;

		//part.mass = glm::gaussRand<float>(500.0f, 60.0f);
		particles->push_back(part);
	}
	std::cout << particles->size();
	
	std::sort(particles->begin(), particles->end(), [](Particle a, Particle b) {return a.cell < b.cell; });

	uint32_t currentCell = 0;
	uint32_t currentOffset = 0;

	offsets->resize(GRID_DIMENSIONS.x * GRID_DIMENSIONS.y * GRID_DIMENSIONS.z);

	

	for (size_t i = 0; i < particles->size(); i++) {
		if ((*particles)[i].cell != currentCell) {
			if (currentCell + 1 == offsets->size()) {
				break;
			}
			(*offsets)[currentCell+1] = currentOffset;
			currentCell++;
			i--;
		}
		else {
			currentOffset++;
		}
	}
	bool flag = false;
	for (size_t i = 0; i < offsets->size(); i++) {
		if (flag == false && (*offsets)[i] > 0) {
			flag = true;
		}
		else if (flag == true && (*offsets)[i] == 0) {
			(*offsets)[i] = (*offsets)[i - 1];
		}
	}
	
}