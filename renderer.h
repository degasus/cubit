#ifndef _RENDERER_H_
#define _RENDERER_H_

class Renderer;

/**
 *
 */
class Renderer {
public:
	/**
	 *
	 */
	Renderer();

	void render(PlayerPosition pos);
	void deleteArea(Area* area);

private:

};

#endif