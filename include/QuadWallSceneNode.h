/* bzflag
 * Copyright 1993-1999, Chris Schoeneman
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* QuadWallSceneNode:
 *	Encapsulates information for rendering a quadrilateral wall.
 */

#ifndef	BZF_QUAD_WALL_SCENE_NODE_H
#define	BZF_QUAD_WALL_SCENE_NODE_H

#include "WallSceneNode.h"

class QuadWallSceneNode : public WallSceneNode {
  public:
			QuadWallSceneNode(const GLfloat base[3],
				const GLfloat sEdge[3],
				const GLfloat tEdge[3],
				float uRepeats = 1.0,
				float vRepeats = 1.0,
				boolean makeLODs = True);
			QuadWallSceneNode(const GLfloat base[3],
				const GLfloat sEdge[3],
				const GLfloat tEdge[3],
				float uOffset,
				float vOffset,
				float uRepeats,
				float vRepeats,
				boolean makeLODs);
			~QuadWallSceneNode();

    int			split(const float*, SceneNode*&, SceneNode*&) const;

    void		addRenderNodes(SceneRenderer&);
    void		addShadowNodes(SceneRenderer&);

  private:
    void		init(const GLfloat base[3],
				const GLfloat uEdge[3],
				const GLfloat vEdge[3],
				float uOffset,
				float vOffset,
				float uRepeats,
				float vRepeats,
				boolean makeLODs);

  protected:
    class Geometry : public RenderNode {
      public:
			Geometry(QuadWallSceneNode*,
				int uCount, int vCount,
				const GLfloat base[3],
				const GLfloat uEdge[3],
				const GLfloat vEdge[3],
				const GLfloat* normal,
				float uOffset, float vOffset,
				float uRepeats, float vRepeats);
			~Geometry();
	void		setStyle(int _style) { style = _style; }
	void		render();
	const GLfloat*	getPosition() { return wall->getSphere(); }
      private:
	void		drawV() const;
	void		drawVT() const;
      private:
	WallSceneNode*	wall;
	int		style;
	int		ds, dt;
	int		dsq, dsr;
	const GLfloat*	normal;
      public:
	GLfloat3Array	vertex;
	GLfloat2Array	uv;
    };

  private:
    Geometry**		nodes;
    Geometry*		shadowNode;
};

#endif // BZF_QUAD_WALL_SCENE_NODE_H
