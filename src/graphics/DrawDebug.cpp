/*
 * Copyright 2011-2012 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "graphics/DrawDebug.h"

#include <sstream>

#include "ai/Paths.h"

#include "animation/AnimationRender.h"

#include "core/Core.h"
#include "core/ArxGame.h"

#include "font/Font.h"

#include "game/Entity.h"
#include "game/EntityManager.h"
#include "game/Inventory.h"
#include "game/NPC.h"
#include "game/Player.h"

#include "graphics/Draw.h"
#include "graphics/DrawLine.h"
#include "graphics/Math.h"
#include "graphics/data/TextureContainer.h"
#include "graphics/effects/Fog.h"
#include "graphics/particle/ParticleEffects.h"
#include "graphics/texture/Texture.h"

#include "gui/Interface.h"
#include "gui/Text.h"

#include "input/Input.h"

#include "math/Vector.h"

#include "physics/Anchors.h"
#include "physics/Collisions.h"

#include "scene/Interactive.h"
#include "scene/Light.h"
#include "scene/Object.h"

extern bool EXTERNALVIEW; // *sigh*

static TextureContainer * g_lightSourceTexture = NULL;
static EERIE_3DOBJ * g_fogObject = NULL;
static EERIE_3DOBJ * g_nodeObject = NULL;

const float DebugTextMaxDistance = 1000.f;
const float DebugPhysicsMaxDistance = 2000.f;

void drawDebugInitialize() {
	g_lightSourceTexture = TextureContainer::LoadUI("graph/particles/light");
	g_fogObject = LoadTheObj("editor/obj3d/fog_generator.teo", "node_teo maps");
	g_nodeObject = LoadTheObj("editor/obj3d/node.teo", "node_teo maps");
}

void drawDebugRelease() {
	delete g_fogObject;
	g_fogObject = NULL;
	delete g_nodeObject;
	g_nodeObject = NULL;
}

enum DebugViewType {
	DebugView_None,
	DebugView_Entities,
	DebugView_Paths,
	DebugView_PathFind,
	DebugView_Lights,
	DebugView_Fogs,
	DebugView_CollisionShapes,
	DebugView_Portals,
	DebugView_Materials,
	DebugViewCount
};

static DebugViewType g_debugView = DebugView_None;

void drawDebugCycleViews() {
	g_debugView = static_cast<DebugViewType>(g_debugView + 1);
	if(g_debugView == DebugViewCount) {
		g_debugView = DebugView_None;
	}
}

static void drawDebugBoundingBox(const EERIE_2D_BBOX & box, Color color = Color::white) {
	if(box.valid()) {
		drawLine2D(box.min.x, box.min.y, box.max.x, box.min.y, 0.01f, color);
		drawLine2D(box.max.x, box.min.y, box.max.x, box.max.y, 0.01f, color);
		drawLine2D(box.max.x, box.max.y, box.min.x, box.max.y, 0.01f, color);
		drawLine2D(box.min.x, box.max.y, box.min.x, box.min.y, 0.01f, color);
	}
}

static void drawDebugLights() {
	
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetCulling(Renderer::CullNone);
	
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	
	for(size_t i = 0; i < MAX_LIGHTS; i++) {
		
		EERIE_LIGHT * light = GLight[i];
		if(!light) {
			continue;
		}
		
		TexturedVertex center;
		EE_RTP(light->pos, &center);
		
		const Rect mouseTestRect(
		center.p.x - 20,
		center.p.y - 20,
		center.p.x + 20,
		center.p.y + 20
		);
		
		if(mouseTestRect.contains(Vec2i(DANAEMouse))) {
			
			Sphere fallstart;
			fallstart.origin = light->pos;
			fallstart.radius = light->fallstart;
			drawLineSphere(fallstart, Color(Color3<u8>::green, 200));
			Sphere fallend;
			fallend.origin = light->pos;
			fallend.radius = light->fallend;
			drawLineSphere(fallend, Color(Color3<u8>::red, 200));
		}
		
		if(light->m_screenRect.valid())
			drawDebugBoundingBox(light->m_screenRect);
	}
	
	for(size_t i = 0; i < MAX_LIGHTS; i++) {
		
		EERIE_LIGHT * light = GLight[i];
		if(!light) {
			continue;
		}
		
		EERIEDrawSprite(light->pos, 11.f, g_lightSourceTexture, light->rgb.to<u8>(), 0.5f);
	}
	
	GRenderer->SetCulling(Renderer::CullCCW);
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
	
}

static void drawDebugPortals() {
	
	GRenderer->SetRenderState(Renderer::Fog, false);
	GRenderer->SetRenderState(Renderer::DepthTest, false);
	
	for(size_t i = 0; i < portals->portals.size(); i++) {
		
		EERIE_PORTALS & po = portals->portals[i];
		
		Color color = Color::red;
		if(po.useportal == 1) {
			color = Color::green;
		}
		
		EERIEPOLY & epp = po.poly;

		drawLine(epp.v[0].p, epp.v[1].p, color);
		drawLine(epp.v[1].p, epp.v[3].p, color);
		drawLine(epp.v[2].p, epp.v[3].p, color);
		drawLine(epp.v[0].p, epp.v[2].p, color);
		
	}
	
	GRenderer->SetRenderState(Renderer::DepthTest, true);
	GRenderer->SetRenderState(Renderer::Fog, true);
	
}

static void drawDebugPaths() {
	
	GRenderer->SetRenderState(Renderer::DepthTest, false);
	
	for(long i = 0; i < nbARXpaths; i++) {
		
		ARX_PATH * path = ARXpaths[i];
		if(!path) {
			continue;
		}
		
		Vec3f center = Vec3f_ZERO;
		int n = 0;
		
		std::vector<Vec3f> points;
		for(long i = 0; i < path->nb_pathways; i++) {
			const ARX_PATHWAY & node = path->pathways[i];
			Vec3f pos = path->pos + node.rpos;
			points.push_back(pos);
			center += pos, n++;
			if(node.flag == PATHWAY_BEZIER) {
				// Interpolate bezier curve by creating linear segments
				if(i + 2 >= path->nb_pathways) {
					break;
				}
				const size_t nsegments = 20;
				for(size_t j = 0; j < nsegments; j++) {
					points.push_back(path->interpolateCurve(i, float(j) / nsegments));
				}
				i++; // Skip the control point
			}
		}
		
		// Zones only check the bounding box for the y coordinate - adjust display for that
		if(path->height > 0) {
			for(size_t i = 0; i < points.size(); i++) {
				points[i].y = path->bbmin.y;
			}
		}
		
		if(path->height != 0 || ((path->flags & PATH_LOOP) && points.size() > 0)) {
			points.push_back(points[0]);
		}
		
		Color color = (path->height != 0) ? Color::green : Color::red;
		
		for(size_t i = 0; i + 1 < points.size(); i++) {
			drawLine(points[i], points[i + 1], color);
		}
		
		if(path->height > 0) {
			Vec3f offset(0.f, (path->bbmax.y - path->bbmin.y), 0.f);
			for(size_t i = 0; i + 1 < points.size(); i++) {
				drawLine(points[i] + offset, points[i + 1] + offset, color);
			}
			for(size_t i = 0; i < points.size(); i++) {
				drawLine(points[i], points[i] + offset, color);
			}
		}
		
		// Display the name and controlling entity for close zones
		if(!path->name.empty() || !path->controled.empty()) {
			if(path->height > 0) {
				center = (path->bbmin + path->bbmax) / 2.f;
			} else if(n != 0) {
				center /= float(n);
			} else {
				center = path->pos;
			}
			if(closerThan(center, player.pos, DebugTextMaxDistance)) {
				std::string controlledby;
				if(!path->controled.empty()) {
					controlledby = "Controlled by: " + path->controled;
				}
				Color textcolor = color * 0.5f + Color::gray(0.5f);
				drawTextAt(hFontDebug, center, path->name, textcolor, controlledby);
			}
		}
		
	}
	
	GRenderer->SetRenderState(Renderer::DepthTest, true);
	
}

static void drawDebugPathFinding() {
	
	if(!ACTIVEBKG || !ACTIVEBKG->anchors) {
		return;
	}
	
	const float zbias = 0.00001f;
	
	for(long i = 0; i < ACTIVEBKG->nbanchors; i++) {
		
		const ANCHOR_DATA & node = ACTIVEBKG->anchors[i];
		
		Color color1 = (node.flags & ANCHOR_FLAG_BLOCKED) ? Color::blue : Color::green;
		for(long j = 0; j < node.nblinked; j++) {
			long k = node.linked[j];
			if(k >= 0 && k < ACTIVEBKG->nbanchors && i < k) {
				const ANCHOR_DATA & other = ACTIVEBKG->anchors[k];
				Color color2 = (other.flags & ANCHOR_FLAG_BLOCKED) ? Color::blue : Color::green;
				drawLine(node.pos, other.pos, color1, color2, zbias);
			}
		}
		
		if(node.height != 0.f) {
			Vec3f toppos = node.pos + Vec3f(0.f, node.height, 0.f);
			drawLine(node.pos, toppos, Color::blue, zbias);
		}
		
	}
	
	// Highlight active paths
	for(size_t i = 1; i < entities.size(); i++) {
		const EntityHandle handle = EntityHandle(i);
		const Entity * entity = entities[handle];
		
		if(!entity || !(entity->ioflags & IO_NPC)) {
			continue;
		}
		const IO_PATHFIND & pathfind = entity->_npcdata->pathfind;
		if(pathfind.listnb <= 0 || !pathfind.list) {
			continue;
		}
		
		// Draw visited nodes yellow and target nodes as red
		for(long j = 1; j < pathfind.listnb; j++) {
			short k0 = pathfind.list[j - 1], k1 = pathfind.list[j];
			if(k0 >= 0 && k0 < ACTIVEBKG->nbanchors && k1 >= 0 && k1 < ACTIVEBKG->nbanchors) {
				const ANCHOR_DATA & n0 = ACTIVEBKG->anchors[k0], & n1 = ACTIVEBKG->anchors[k1];
				Color color0 = (j     <= pathfind.listpos) ? Color::yellow : Color::red;
				Color color1 = (j + 1 <= pathfind.listpos) ? Color::yellow : Color::red;
				drawLine(n0.pos, n1.pos, color0, color1, 2.f * zbias);
			}
		}
		
		// Highlight end nodes
		short k0 = pathfind.list[pathfind.listnb - 1];
		if(k0 >= 0 && k0 < ACTIVEBKG->nbanchors) {
			Anglef angle(0.f, 0.f, 0.f);
			Vec3f scale(0.5f);
			RenderMaterial mat;
			mat.setBlendType(RenderMaterial::Opaque);
			mat.setDepthTest(true);
			
			Draw3DObject(g_nodeObject, angle, ACTIVEBKG->anchors[k0].pos, scale, Color3f::white, mat);
		}
		
		// Show entity ID at the active node
		if(pathfind.listpos < pathfind.listnb) {
			short k1 = pathfind.list[pathfind.listpos];
			if(k1 >= 0 && k1 < ACTIVEBKG->nbanchors) {
				if(closerThan(ACTIVEBKG->anchors[k1].pos, player.pos, DebugTextMaxDistance)) {
					drawTextAt(hFontDebug, ACTIVEBKG->anchors[k1].pos, entity->idString());
					GRenderer->SetRenderState(Renderer::DepthTest, true);
				}
			}
		}
		
	}
	
}

static void drawDebugFogs() {
	
	for(size_t i = 0; i < MAX_FOG; i++) {
		
		FOG_DEF * fog = &fogs[i];
		if(!fog->exist) {
			continue;
		}
		
			Anglef angle(0.f, 0.f, 0.f);
			Vec3f scale(1.f);
			RenderMaterial mat;
			mat.setBlendType(RenderMaterial::Opaque);
			mat.setDepthTest(true);
			
			Draw3DObject(g_fogObject, angle, fog->pos, scale, Color3f::white, mat);
		
		if(fog->special & FOG_DIRECTIONAL) {
			drawLine(fog->pos, fog->pos + fog->move * 50.f, Color::white);
		}
		
		Sphere fogsize;
		fogsize.origin = fog->pos;
		fogsize.radius = fog->size;
		drawLineSphere(fogsize, Color(Color3<u8>::blue, 200));
		
	}
	
}

//! Debug function to show the physical box of an object
static void drawDebugCollisionShape(EERIE_3DOBJ * obj) {
	
	if(!obj || !obj->pbox) {
		return;
	}
	
	Sphere sphere;
	sphere.origin = obj->pbox->vert[0].pos;
	sphere.radius = obj->pbox->radius;
	drawLineSphere(sphere, Color::white);
	
	GRenderer->SetRenderState(Renderer::DepthTest, false);
	
	Color shapeColor = Color::yellow;
	
	if(obj->pbox->active == 2) {
		shapeColor = Color::green;
	}
	
	for(long k = 0; k + 1 < obj->pbox->nb_physvert; k++) {
		drawLine(obj->pbox->vert[k].pos, obj->pbox->vert[k+1].pos, shapeColor);
	}
	
}

static void drawDebugEntityPhysicsCylinder(Entity * io) {
	
	if(!(io->ioflags & IO_NPC)) {
		return;
	}
	
	CollisionFlags levitate = 0;
	
	if(spells.getSpellOnTarget(io->index(), SPELL_LEVITATE)) {
		levitate = CFLAG_LEVITATE;
	}
	
	Cylinder cyll;
	cyll.height = GetIOHeight(io);
	cyll.radius = GetIORadius(io);
	cyll.origin = io->physics.startpos;
	drawLineCylinder(cyll, Color::green);
	
	if(!(AttemptValidCylinderPos(cyll, io, levitate | CFLAG_NPC))) {
		cyll.height = -40.f;
		drawLineCylinder(cyll, Color::blue);
		cyll.height = GetIOHeight(io);
	}
	
	cyll.origin = io->physics.targetpos;
	drawLineCylinder(cyll, Color::red);
	
}

static void drawDebugEntityPhysicsCylinders() {
	for(size_t i = 1; i < entities.size(); i++) {
		const EntityHandle handle = EntityHandle(i);
		Entity * entity = entities[handle];
		
		if(!entity || !closerThan(entity->pos, player.pos, DebugPhysicsMaxDistance))
			continue;
		
		drawDebugCollisionShape(entity->obj);
		drawDebugEntityPhysicsCylinder(entity);
	}
}

static void drawDebugEntities() {
	
	for(size_t i = 1; i < entities.size(); i++) {
		const EntityHandle handle = EntityHandle(i);
		Entity * entity = entities[handle];
		
		if(!entity) {
			continue;
		}
		
		Color color = Color::white;
		bool visible = true;
		switch(entity->show) {
			case SHOW_FLAG_DESTROYED:    continue; // Don't even display the name
			case SHOW_FLAG_IN_INVENTORY: continue;
			case SHOW_FLAG_ON_PLAYER:    continue;
			case SHOW_FLAG_LINKED:       continue;
			case SHOW_FLAG_NOT_DRAWN:    color = Color::magenta; visible = false; break;
			case SHOW_FLAG_HIDDEN:       color = Color::yellow;  visible = false; break;
			case SHOW_FLAG_MEGAHIDE:     color = Color::green;   visible = false; break;
			case SHOW_FLAG_KILLED:       color = Color::red;     visible = false; break;
			case SHOW_FLAG_IN_SCENE:     color = Color::white;   visible = true;  break;
			case SHOW_FLAG_TELEPORTING:  color = Color::blue;    visible = true;  break;
		}
		if((entity->ioflags & IO_CAMERA) || (entity->ioflags & IO_MARKER)) {
			color = Color::gray(0.7f), visible = false;
		}
		if(DRAGINTER == entity) {
			color = Color::white, visible = true;
		}
		
		if(visible) {
			drawDebugBoundingBox(entity->bbox2D, Color::blue);
		}
		
		if(closerThan(entity->pos, player.pos, DebugTextMaxDistance)) {
			
			if(visible && entity->bbox2D.valid()) {
				int x = (entity->bbox2D.min.x + entity->bbox2D.max.x) / 2;
				int y = entity->bbox2D.min.y - hFontDebug->getLineHeight() - 2;
				UNICODE_ARXDrawTextCenter(hFontDebug, Vec2f(x, y), entity->idString(), color);
			} else {
				drawTextAt(hFontDebug, entity->pos, entity->idString(), color);
			}
			
			if(entity->obj) {
				for(size_t j = 0; j < entity->obj->linked.size(); j++) {
					Vec3f pos = entity->obj->vertexlist3[entity->obj->linked[j].lidx].v;
					Entity * other = entity->obj->linked[j].io;
					drawTextAt(hFontDebug, pos, other->idString(), Color::cyan);
				}
			}
			
		}
		
	}
	
}

/*!
 * Check if \ref p is in the 2D-triangle defined by the x an y components of
 * \ref a, \ref b and \ref c - and if so, interpolate the z components for
 * p's position in the triangle.
 * \return the interpolated z position, or \c -1 if p is not in the triangle.
 */
static float pointInTriangle(Vec2f p, Vec3f a, Vec3f b, Vec3f c) {
	
	Vec3f d0 = c - a, d1 = b - a;
	Vec2f v0 = Vec2f(d0.x, d0.y), v1 = Vec2f(d1.x, d1.y);
	Vec2f v2 = p - Vec2f(a.x, a.y);
	
	float dot00 = glm::dot(v0, v0);
	float dot01 = glm::dot(v0, v1);
	float dot02 = glm::dot(v0, v2);
	float dot11 = glm::dot(v1, v1);
	float dot12 = glm::dot(v1, v2);
	
	float scale = 1 / (dot00 * dot11 - dot01 * dot01);
	float u = (dot11 * dot02 - dot01 * dot12) * scale;
	float v = (dot00 * dot12 - dot01 * dot02) * scale;
	
	if(u >= 0 && v >= 0 && u + v < 1) {
		return a.z + d0.z * u + d1.z * v;
	} else {
		return -1.f;
	}
}

static void drawDebugMaterialTexture(Vec2f & textpos, const std::string & type,
                                     const Texture2D & t, Color color) {
	
	const std::string & name = t.getFileName().string();
	
	std::ostringstream oss;
	oss << "(" << t.GetFormat() << ", " << t.getSize().x << "×" << t.getSize().y;
	if(t.getStoredSize() != t.getSize()) {
		oss << " [" << t.getStoredSize().x << "×" << t.getStoredSize().y << "]";
	}
	if(t.hasMipmaps()) {
		oss << " + mip";
	}
	oss << ")";
	std::string format = oss.str();
	
	float type_s = hFontDebug->getTextSize(type).x + 10;
	float name_s = hFontDebug->getTextSize(name).x + 10;
	float format_s = hFontDebug->getTextSize(format).x;
	
	Vec2i pos = Vec2i(textpos);
	pos.x -= (type_s + name_s + format_s) * 0.5f;
	if(pos.x < g_size.left + 5) {
		pos.x = g_size.left + 5;
	} else if(pos.x + type_s + name_s + format_s > g_size.right - 5) {
		pos.x = g_size.right - 5 - (type_s + name_s + format_s);
	}
	
	hFontDebug->draw(pos + Vec2i_ONE, type, Color::black);
	hFontDebug->draw(pos, type, color);
	pos.x += type_s;
	
	hFontDebug->draw(pos + Vec2i_ONE, name, Color::black);
	hFontDebug->draw(pos, name, Color::white);
	pos.x += name_s;
	
	hFontDebug->draw(pos + Vec2i_ONE, format, Color::black);
	hFontDebug->draw(pos, format, Color::gray(0.7f));
	
	textpos.y += hFontDebug->getLineHeight();
}

static void drawDebugMaterials() {
	
	if(!ACTIVEBKG || !ACTIVEBKG->exist) {
		return;
	}
	
	PolyType skip = POLY_NODRAW | POLY_HIDE;
	if(GInput->isKeyPressed(Keyboard::Key_LeftShift)) {
		skip |= POLY_TRANS | POLY_WATER;
	}
	
	Vec2f point(DANAEMouse);
	
	Entity * owner = GetFirstInterAtPos(Vec2s(point));
	TextureContainer * material = NULL;
	size_t count = 0;
	Vec2f pp[4];
	Vec2f puv[4];
	PolyType flags;
	
	float minz = std::numeric_limits<float>::max();
	
	for(size_t k = 1; k < entities.size(); k++) {
		
		EntityHandle h = EntityHandle(k);
		if(!entities[h] || !entities[h]->obj) {
			continue;
		}
		Entity * entity = entities[h];
		
		if((entity->ioflags & IO_CAMERA) || (entity->ioflags & IO_MARKER))
			continue;
		
		if(!(entity->gameFlags & GFLAG_ISINTREATZONE))
			continue;
		if((entity->gameFlags & GFLAG_INVISIBILITY))
			continue;
		if((entity->gameFlags & GFLAG_MEGAHIDE))
			continue;
		
		switch(entity->show) {
			case SHOW_FLAG_DESTROYED:    continue;
			case SHOW_FLAG_IN_INVENTORY: continue;
			case SHOW_FLAG_ON_PLAYER:    continue;
			case SHOW_FLAG_LINKED:       break;
			case SHOW_FLAG_NOT_DRAWN:    continue;
			case SHOW_FLAG_HIDDEN:       continue;
			case SHOW_FLAG_MEGAHIDE:     continue;
			case SHOW_FLAG_KILLED:       continue;
			case SHOW_FLAG_IN_SCENE:     break;
			case SHOW_FLAG_TELEPORTING:  break;
		}
		
		if(!entity->bbox2D.valid()) {
			continue;
		}
		
		for(size_t j = 0; j < entity->obj->facelist.size(); j++) {
			const EERIE_FACE & face = entity->obj->facelist[j];
			
			if(face.facetype & skip) {
				continue;
			}
			
			bool valid = true;
			bool bvalid = false;
			Vec3f p[3];
			Vec2f uv[3];
			for(size_t i = 0; i < 3; i++) {
				unsigned short v = face.vid[i];
				valid = valid && v < entity->obj->vertexlist3.size();
				if(valid) {
					if(entity->animlayer[0].cur_anim) {
						p[i] = entity->obj->vertexlist3[v].vert.p;
						uv[i] = entity->obj->vertexlist3[v].vert.uv;
					} else {
						p[i] = entity->obj->vertexlist[v].vert.p;
						uv[i] = entity->obj->vertexlist[v].vert.uv;
					}
					valid = valid && (p[i].z > 0.000001f);
					bvalid = bvalid || (p[i].x >= g_size.left && p[i].x < g_size.right
					                 && p[i].y >= g_size.top && p[i].y < g_size.bottom);
				}
			}
			
			if(!valid || !bvalid) {
				continue;
			}
			
			float z = pointInTriangle(point, p[0], p[1], p[2]);
			
			if(z > 0 && z <= minz) {
				count = 3;
				for(size_t i = 0; i < count; i++) {
					pp[i] = Vec2f(p[i].x, p[i].y);
					puv[i] = uv[i];
				}
				if(face.texid >= 0 && size_t(face.texid) < entity->obj->texturecontainer.size()) {
					material = entity->obj->texturecontainer[face.texid];
				} else {
					material = NULL;
				}
				owner = entity;
				minz = z;
				flags = face.facetype & ~(POLY_WATER | POLY_LAVA);
			}
			
		}
	}
	
	for(short z = 0; z < ACTIVEBKG->Zsize; z++)
	for(short x = 0; x < ACTIVEBKG->Xsize; x++) {
		const EERIE_BKG_INFO & feg = ACTIVEBKG->fastdata[x][z];
		
		if(!feg.treat) {
			continue;
		}
		
		for(long l = 0; l < feg.nbpolyin; l++) {
			EERIEPOLY * ep = feg.polyin[l];
			
			if(!ep) {
				continue;
			}
			
			if(ep->type & skip) {
				continue;
			}
			
			bool valid = true;
			bool bvalid = false;
			Vec3f p[4];
			for(size_t i = 0; i < ((ep->type & POLY_QUAD) ? 4 : 3); i++) {
				TexturedVertex tv;
				tv.p = EE_RT(ep->v[i].p);
				valid = valid && (tv.p.z > 0.000001f);
				EE_P(&tv.p, &tv);
				bvalid = bvalid || (tv.p.x >= g_size.left && tv.p.x < g_size.right
				                 && tv.p.y >= g_size.top && tv.p.y < g_size.bottom);
				p[i] = tv.p;
			}
			
			if(!valid || !bvalid) {
				continue;
			}
			
			float z = pointInTriangle(point, p[0], p[1], p[2]);
			if(z <= 0 && (ep->type & POLY_QUAD)) {
				z = pointInTriangle(point, p[1], p[3], p[2]);
			}
			
			if(z > 0 && z <= minz) {
				count = ((ep->type & POLY_QUAD) ? 4 : 3);
				for(size_t i = 0; i < count; i++) {
					pp[i] = Vec2f(p[i].x, p[i].y);
					puv[i] = ep->v[i].uv;
				}
				material = ep->tex;
				owner = NULL;
				minz = z;
				flags = ep->type;
			}
			
		}
	}
	
	if(count) {
		
		GRenderer->SetRenderState(Renderer::DepthTest, false);
		
		drawLine2D(pp[0], pp[1], 0.1f, Color::magenta);
		drawLine2D(pp[2], pp[0], 0.1f, Color::magenta);
		if(count == 4) {
			drawLine2D(pp[2], pp[3], 0.1f, Color::magenta);
			drawLine2D(pp[3], pp[1], 0.1f, Color::magenta);
		} else {
			drawLine2D(pp[1], pp[2], 0.1f, Color::magenta);
		}
		
		Vec2f c = Vec2f(0.f);
		float miny = std::numeric_limits<float>::max();
		float maxy = std::numeric_limits<float>::min();
		for(size_t i = 0; i < count; i++) {
			c += pp[i];
			miny = std::min(miny, pp[i].y);
			maxy = std::max(maxy, pp[i].y);
		}
		c *= 1.f / count;
		
		Vec2f textpos(c.x, miny - 2 * hFontDebug->getLineHeight());
		
		std::ostringstream oss;
		
		if(owner) {
			textpos.y -= hFontDebug->getLineHeight();
		}
		if(material && material->m_pTexture) {
			textpos.y -= hFontDebug->getLineHeight();
		}
		if((flags & (POLY_WATER | POLY_LAVA)) && enviro && enviro->m_pTexture) {
			textpos.y -= hFontDebug->getLineHeight();
		}
		if(textpos.y < g_size.top + 5) {
			textpos.y = maxy + 2 * hFontDebug->getLineHeight();
		}
		
		
		if(owner) {
			drawTextCentered(hFontDebug, textpos, owner->idString(), Color::cyan);
			textpos.y += hFontDebug->getLineHeight();
		}
		if(material && material->m_pTexture) {
			drawDebugMaterialTexture(textpos, "Diffuse: ", *material->m_pTexture, Color::green);
		}
		if((flags & (POLY_WATER | POLY_LAVA)) && enviro && enviro->m_pTexture) {
			oss.str(std::string());
			oss << "Animation: ";
			oss << ((flags & (POLY_LAVA)) ? "lava" : "water");
			if(flags & POLY_FALL) {
				oss << " (flowing)";
			}
			drawDebugMaterialTexture(textpos, oss.str(), *enviro->m_pTexture, Color::yellow);
		}
		
		(void)textpos;
		
		for(size_t i = 0; i < count; i++) {
			
			oss.str(std::string());
			oss.setf(std::ios_base::fixed, std::ios_base::floatfield);
			oss.precision(2);
			oss << '(' << puv[i].x << ',' << puv[i].y << ')';
			std::string text = oss.str();
			
			Vec2f textpos = pp[i];
			if(pp[i].y < c.y) {
				textpos.y -= hFontDebug->getLineHeight();
			}
			
			if(pp[i].x < c.x) {
				Vec2i size = hFontDebug->getTextSize(text);
				textpos.x -= size.x;
			}
			
			hFontDebug->draw(textpos.x, textpos.y, text, Color::gray(0.7f));
		}
		
		GRenderer->SetRenderState(Renderer::DepthTest, true);
		
	}
	
}

void drawDebugRender() {
	
	if(g_debugView == DebugView_None) {
		return;
	}
	
	std::stringstream ss;
	ss << "Debug Display: ";
	
	switch(g_debugView) {
		case DebugView_Entities: {
			ss << "Entities";
			drawDebugEntities();
			break;
		}
		case DebugView_Paths: {
			ss << "Paths and Zones";
			drawDebugPaths();
			break;
		}
		case DebugView_PathFind: {
			ss << "Pathfinding";
			drawDebugPathFinding();
			break;
		}
		case DebugView_Lights: {
			ss << "Lights";
			drawDebugLights();
			break;
		}
		case DebugView_Fogs: {
			drawDebugFogs();
			ss << "Fogs";
			break;
		}
		case DebugView_CollisionShapes: {
			ss << "Collision Shapes";
			drawDebugEntityPhysicsCylinders();
			break;
		}
		case DebugView_Portals: {
			ss << "Portals";
			drawDebugPortals();
			break;
		}
		case DebugView_Materials: {
			ss << "Materials";
			drawDebugMaterials();
			break;
		}
		default: return;
	}
	
	s32 width = hFontDebug->getTextSize(ss.str()).x;
	Vec2f pos = Vec2f(g_size.topRight());
	pos += Vec2f(-10 , 10);
	pos += Vec2f(-width, 0);
	
	ARX_TEXT_Draw(hFontDebug, pos, ss.str(), Color::yellow);
}
