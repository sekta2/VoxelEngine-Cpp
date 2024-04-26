#include "Chunks.h"
#include "Chunk.h"
#include "voxel.h"
#include "Block.h"
#include "WorldGenerator.h"
#include "../content/Content.h"
#include "../lighting/Lightmap.h"
#include "../files/WorldFiles.h"
#include "../world/LevelEvents.h"
#include "../graphics/core/Mesh.hpp"
#include "../maths/voxmaths.h"
#include "../maths/aabb.h"
#include "../maths/rays.h"

#include <math.h>
#include <limits.h>
#include <vector>

Chunks::Chunks(uint32_t w, uint32_t d, 
			   int32_t ox, int32_t oz, 
			   WorldFiles* wfile, 
			   LevelEvents* events, 
			   const Content* content) 
		: contentIds(content->getIndices()), 
          chunks(w*d),
          chunksSecond(w*d),
		  w(w), d(d), ox(ox), oz(oz), 
		  worldFiles(wfile), 
		  events(events) {
	volume = (size_t)w*(size_t)d;
	chunksCount = 0;
}

voxel* Chunks::get(int32_t x, int32_t y, int32_t z) {
	x -= ox * CHUNK_W; 
	z -= oz * CHUNK_D;
	int cx = floordiv(x, CHUNK_W);
	int cy = floordiv(y, CHUNK_H);
	int cz = floordiv(z, CHUNK_D);
	if (cx < 0 || cy < 0 || cz < 0 || cx >= int(w) || cy >= 1 || cz >= int(d))
		return nullptr;
	std::shared_ptr<Chunk> chunk = chunks[cz * w + cx];
	if (chunk == nullptr)
		return nullptr;
	int lx = x - cx * CHUNK_W;
	int ly = y - cy * CHUNK_H;
	int lz = z - cz * CHUNK_D;
	return &chunk->voxels[(ly * CHUNK_D + lz) * CHUNK_W + lx];
}

const AABB* Chunks::isObstacleAt(float x, float y, float z){
	int ix = floor(x);
	int iy = floor(y);
	int iz = floor(z);
	voxel* v = get(ix, iy, iz);
	if (v == nullptr) {
		if (iy >= CHUNK_H) {
			return nullptr;
		} else {
        	static const AABB empty;
			return &empty;
		}
    }
	const Block* def = contentIds->getBlockDef(v->id);
	if (def->obstacle) {
        const auto& boxes = def->rotatable 
                         ? def->rt.hitboxes[v->rotation()] 
                         : def->hitboxes;
        for (const auto& hitbox : boxes) {
            if (hitbox.contains({x - ix, y - iy, z - iz}))
                return &hitbox;
        }
	}
	return nullptr;
}

bool Chunks::isSolidBlock(int32_t x, int32_t y, int32_t z) {
    voxel* v = get(x, y, z);
    if (v == nullptr)
        return false;
    return contentIds->getBlockDef(v->id)->rt.solid;
}

bool Chunks::isReplaceableBlock(int32_t x, int32_t y, int32_t z) {
    voxel* v = get(x, y, z);
    if (v == nullptr)
        return false;
    return contentIds->getBlockDef(v->id)->replaceable;
}

bool Chunks::isObstacleBlock(int32_t x, int32_t y, int32_t z) {
	voxel* v = get(x, y, z);
	if (v == nullptr)
		return false;
	return contentIds->getBlockDef(v->id)->obstacle;
}

ubyte Chunks::getLight(int32_t x, int32_t y, int32_t z, int channel){
	x -= ox * CHUNK_W;
	z -= oz * CHUNK_D;
	int cx = floordiv(x, CHUNK_W);
	int cy = floordiv(y, CHUNK_H);
	int cz = floordiv(z, CHUNK_D);
	if (cx < 0 || cy < 0 || cz < 0 || cx >= int(w) || cy >= 1 || cz >= int(d))
		return 0;
	auto chunk = chunks[(cy * d + cz) * w + cx];
	if (chunk == nullptr)
		return 0;
	int lx = x - cx * CHUNK_W;
	int ly = y - cy * CHUNK_H;
	int lz = z - cz * CHUNK_D;
	return chunk->lightmap.get(lx, ly, lz, channel);
}

light_t Chunks::getLight(int32_t x, int32_t y, int32_t z){
	x -= ox * CHUNK_W;
	z -= oz * CHUNK_D;
	int cx = floordiv(x, CHUNK_W);
	int cy = floordiv(y, CHUNK_H);
	int cz = floordiv(z, CHUNK_D);
	if (cx < 0 || cy < 0 || cz < 0 || cx >= int(w) || cy >= 1 || cz >= int(d))
		return 0;
	auto chunk = chunks[(cy * d + cz) * w + cx];
	if (chunk == nullptr)
		return 0;
	int lx = x - cx * CHUNK_W;
	int ly = y - cy * CHUNK_H;
	int lz = z - cz * CHUNK_D;
	return chunk->lightmap.get(lx,ly,lz);
}

Chunk* Chunks::getChunkByVoxel(int32_t x, int32_t y, int32_t z){
	if (y < 0 || y >= CHUNK_H)
		return nullptr;
	x -= ox * CHUNK_W;
	z -= oz * CHUNK_D;
	int cx = floordiv(x, CHUNK_W);
	int cz = floordiv(z, CHUNK_D);
	if (cx < 0 || cz < 0 || cx >= int(w) || cz >= int(d))
		return nullptr;
	return chunks[cz * w + cx].get();
}

Chunk* Chunks::getChunk(int x, int z){
	x -= ox;
	z -= oz;
	if (x < 0 || z < 0 || x >= int(w) || z >= int(d))
		return nullptr;
	return chunks[z * w + x].get();
}

void Chunks::set(int32_t x, int32_t y, int32_t z, uint32_t id, uint8_t states){
	if (y < 0 || y >= CHUNK_H)
		return;
	x -= ox * CHUNK_W;
	z -= oz * CHUNK_D;
	int cx = floordiv(x, CHUNK_W);
	int cz = floordiv(z, CHUNK_D);
	if (cx < 0 || cz < 0 || cx >= int(w) || cz >= int(d))
		return;
	Chunk* chunk = chunks[cz * w + cx].get();
	if (chunk == nullptr)
		return;
	int lx = x - cx * CHUNK_W;
	int lz = z - cz * CHUNK_D;
    
    voxel& vox = chunk->voxels[(y * CHUNK_D + lz) * CHUNK_W + lx]; 
	auto def = contentIds->getBlockDef(vox.id);
	if (def->inventorySize == 0)
		chunk->removeBlockInventory(lx, y, lz);
	vox.id = id;
	vox.states = states;

	chunk->setUnsaved(true);
	chunk->setModified(true);

	if (y < chunk->bottom) chunk->bottom = y;
	else if (y + 1 > chunk->top) chunk->top = y + 1;
	else if (id == 0) chunk->updateHeights();

	if (lx == 0 && (chunk = getChunk(cx+ox-1, cz+oz)))
		chunk->setModified(true);
	if (lz == 0 && (chunk = getChunk(cx+ox, cz+oz-1))) 
		chunk->setModified(true);

	if (lx == CHUNK_W-1 && (chunk = getChunk(cx+ox+1, cz+oz))) 
		chunk->setModified(true);
	if (lz == CHUNK_D-1 && (chunk = getChunk(cx+ox, cz+oz+1))) 
		chunk->setModified(true);
}

voxel* Chunks::rayCast(glm::vec3 start, 
					   glm::vec3 dir, 
					   float maxDist, 
					   glm::vec3& end, 
					   glm::ivec3& norm, 
					   glm::ivec3& iend) {
	float px = start.x;
	float py = start.y;
	float pz = start.z;

	float dx = dir.x;
	float dy = dir.y;
	float dz = dir.z;

	float t = 0.0f;
	int ix = floor(px);
	int iy = floor(py);
	int iz = floor(pz);

	int stepx = (dx > 0.0f) ? 1 : -1;
	int stepy = (dy > 0.0f) ? 1 : -1;
	int stepz = (dz > 0.0f) ? 1 : -1;

	constexpr float infinity = std::numeric_limits<float>::infinity();

	float txDelta = (dx == 0.0f) ? infinity : abs(1.0f / dx);
	float tyDelta = (dy == 0.0f) ? infinity : abs(1.0f / dy);
	float tzDelta = (dz == 0.0f) ? infinity : abs(1.0f / dz);

	float xdist = (stepx > 0) ? (ix + 1 - px) : (px - ix);
	float ydist = (stepy > 0) ? (iy + 1 - py) : (py - iy);
	float zdist = (stepz > 0) ? (iz + 1 - pz) : (pz - iz);

	float txMax = (txDelta < infinity) ? txDelta * xdist : infinity;
	float tyMax = (tyDelta < infinity) ? tyDelta * ydist : infinity;
	float tzMax = (tzDelta < infinity) ? tzDelta * zdist : infinity;

	int steppedIndex = -1;      
                                
	while (t <= maxDist){       
		voxel* voxel = get(ix, iy, iz);		
		if (voxel == nullptr){ return nullptr; }

		const Block* def = contentIds->getBlockDef(voxel->id);
		if (def->selectable){
			end.x = px + t * dx;
			end.y = py + t * dy;
			end.z = pz + t * dz;
					iend.x = ix;
					iend.y = iy;
					iend.z = iz;
			
			if (!def->rt.solid) {
                const std::vector<AABB>& hitboxes = def->rotatable
                        ? def->rt.hitboxes[voxel->rotation()]
                        : def->hitboxes;

                scalar_t distance = maxDist;
                Ray ray(start, dir);

                bool hit = false;

                for (const auto& box : hitboxes) {
                    scalar_t boxDistance;
					glm::ivec3 boxNorm;
                    if (ray.intersectAABB(iend, box, maxDist, boxNorm, boxDistance) > RayRelation::None && boxDistance < distance) {
                        hit = true;
                        distance = boxDistance;
                        norm = boxNorm;
                        end = start + (dir * glm::vec3(distance));
                    }
                }

                if (hit) return voxel;
			} else {
				iend.x = ix;
				iend.y = iy;
				iend.z = iz;

				norm.x = norm.y = norm.z = 0;
				if (steppedIndex == 0) norm.x = -stepx;
				if (steppedIndex == 1) norm.y = -stepy;
				if (steppedIndex == 2) norm.z = -stepz;
				return voxel;
			}
		}
		if (txMax < tyMax) {
			if (txMax < tzMax) {
				ix += stepx;
				t = txMax;
				txMax += txDelta;
				steppedIndex = 0;
			} else {
				iz += stepz;
				t = tzMax;
				tzMax += tzDelta;
				steppedIndex = 2;
			}
		} else {
			if (tyMax < tzMax) {
				iy += stepy;
				t = tyMax;
				tyMax += tyDelta;
				steppedIndex = 1;
			} else {
				iz += stepz;
				t = tzMax;
				tzMax += tzDelta;
				steppedIndex = 2;
			}
		}
	}
	iend.x = ix;
	iend.y = iy;
	iend.z = iz;

	end.x = px + t * dx;
	end.y = py + t * dy;
	end.z = pz + t * dz;
	norm.x = norm.y = norm.z = 0;
	return nullptr;
}

glm::vec3 Chunks::rayCastToObstacle(glm::vec3 start, glm::vec3 dir, float maxDist) {
	float px = start.x;
	float py = start.y;
	float pz = start.z;

	float dx = dir.x;
	float dy = dir.y;
	float dz = dir.z;

	float t = 0.0f;
	int ix = floor(px);
	int iy = floor(py);
	int iz = floor(pz);

	int stepx = (dx > 0.0f) ? 1 : -1;
	int stepy = (dy > 0.0f) ? 1 : -1;
	int stepz = (dz > 0.0f) ? 1 : -1;

	constexpr float infinity = std::numeric_limits<float>::infinity();

	float txDelta = (dx == 0.0f) ? infinity : abs(1.0f / dx);
	float tyDelta = (dy == 0.0f) ? infinity : abs(1.0f / dy);
	float tzDelta = (dz == 0.0f) ? infinity : abs(1.0f / dz);

	float xdist = (stepx > 0) ? (ix + 1 - px) : (px - ix);
	float ydist = (stepy > 0) ? (iy + 1 - py) : (py - iy);
	float zdist = (stepz > 0) ? (iz + 1 - pz) : (pz - iz);

	float txMax = (txDelta < infinity) ? txDelta * xdist : infinity;
	float tyMax = (tyDelta < infinity) ? tyDelta * ydist : infinity;
	float tzMax = (tzDelta < infinity) ? tzDelta * zdist : infinity;

	while (t <= maxDist) {
		voxel* voxel = get(ix, iy, iz);
		if (voxel == nullptr) { return glm::vec3(px + t * dx, py + t * dy, pz + t * dz); }

		const Block* def = contentIds->getBlockDef(voxel->id);
		if (def->obstacle) {
			if (!def->rt.solid) {
                const std::vector<AABB>& hitboxes = def->rotatable
                    ? def->rt.hitboxes[voxel->rotation()]
                    : def->modelBoxes;

                scalar_t distance;
                glm::ivec3 norm;
                Ray ray(start, dir);

                for (const auto& box : hitboxes) {
                    // norm is dummy now, can be inefficient
                    if (ray.intersectAABB(glm::ivec3(ix, iy, iz), box, maxDist, norm, distance) > RayRelation::None) {
                        return start + (dir * glm::vec3(distance));
                    }
                }
			}
			else {
				return glm::vec3(px + t * dx, py + t * dy, pz + t * dz);
			}
		}
		if (txMax < tyMax) {
			if (txMax < tzMax) {
				ix += stepx;
				t = txMax;
				txMax += txDelta;
			}
			else {
				iz += stepz;
				t = tzMax;
				tzMax += tzDelta;
			}
		}
		else {
			if (tyMax < tzMax) {
				iy += stepy;
				t = tyMax;
				tyMax += tyDelta;
			}
			else {
				iz += stepz;
				t = tzMax;
				tzMax += tzDelta;
			}
		}
	}
	return glm::vec3(px + maxDist * dx, py + maxDist * dy, pz + maxDist * dz);
}


void Chunks::setCenter(int32_t x, int32_t z) {
	int cx = floordiv(x, CHUNK_W);
	int cz = floordiv(z, CHUNK_D);
	cx -= ox + w / 2;
	cz -= oz + d / 2;
	if (cx | cz) {
		translate(cx,cz);
	}
}

void Chunks::translate(int32_t dx, int32_t dz) {
    auto& regions = worldFiles->getRegions();
	for (uint i = 0; i < volume; i++){
		chunksSecond[i] = nullptr;
	}
	for (uint32_t z = 0; z < d; z++){
		for (uint32_t x = 0; x < w; x++){
			auto chunk = chunks[z * w + x];
			int nx = x - dx;
			int nz = z - dz;
			if (chunk == nullptr)
				continue;
			if (nx < 0 || nz < 0 || nx >= int(w) || nz >= int(d)){
				events->trigger(EVT_CHUNK_HIDDEN, chunk.get());
				if (worldFiles) {
					regions.put(chunk.get());
                }
				chunksCount--;
				continue;
			}
			chunksSecond[nz * w + nx] = chunk;
		}
	}
	std::swap(chunks, chunksSecond);

	ox += dx;
	oz += dz;
}

void Chunks::resize(uint32_t newW, uint32_t newD) {
	if (newW < w) {
		int delta = w - newW;
		translate(delta / 2, 0);
		translate(-delta, 0);
		translate(delta, 0);
	}
	if (newD < d) {
		int delta = d - newD;
		translate(0, delta / 2);
		translate(0, -delta);
		translate(0, delta);
	}
	const int newVolume = newW * newD;
    std::vector<std::shared_ptr<Chunk>> newChunks(newVolume);
    std::vector<std::shared_ptr<Chunk>> newChunksSecond(newVolume);
	for (int z = 0; z < int(d) && z < int(newD); z++) {
		for (int x = 0; x < int(w) && x < int(newW); x++) {
			newChunks[z * newW + x] = chunks[z * w + x];
		}
	}
    w = newW;
    d = newD;
    volume = newVolume;
    chunks = std::move(newChunks);
    chunksSecond = std::move(newChunksSecond);
}

void Chunks::_setOffset(int32_t x, int32_t z) {
	ox = x;
	oz = z;
}

bool Chunks::putChunk(std::shared_ptr<Chunk> chunk) {
	int x = chunk->x;
	int z = chunk->z;
	x -= ox;
	z -= oz;
	if (x < 0 || z < 0 || x >= int(w) || z >= int(d))
		return false;
	chunks[z * w + x] = chunk;
	chunksCount++;
	return true;
}

void Chunks::saveAndClear(){
    auto& regions = worldFiles->getRegions();
	for (size_t i = 0; i < volume; i++){
		Chunk* chunk = chunks[i].get();
		chunks[i] = nullptr;
		if (chunk == nullptr || !chunk->isLighted())
            continue;
        
		bool lightsUnsaved = !chunk->isLoadedLights() && 
                              worldFiles->doesWriteLights();
        if (!chunk->isUnsaved() && !lightsUnsaved)
            continue;
        regions.put(chunk);
	}
	chunksCount = 0;
}
