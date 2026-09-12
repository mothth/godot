/**************************************************************************/
/*  sub_world.h                                                           */
/**************************************************************************/
#pragma once

#include "scene/main/node.h"

#ifndef _3D_DISABLED
class World3D;
#endif // _3D_DISABLED

class Viewport;

class SubWorld : public Node {
	GDCLASS(SubWorld, Node);

protected:

	friend class Viewport;

#ifndef _3D_DISABLED
	Ref<World3D> world_3d;
	Ref<World3D> current_world_3d;
#endif // _3D_DISABLED

	static void _bind_methods();
	void _notification(int p_what);

#ifndef _3D_DISABLED
	void _propagate_exit_world_3d(Node *p_node);
	void _propagate_enter_world_3d(Node *p_node);
	void _viewport_exited_world_3d();
	void _viewport_entered_world_3d();
#endif // _3D_DISABLED

public:

#ifndef _3D_DISABLED
	void set_world_3d(const Ref<World3D> &p_world_3d);
	Ref<World3D> get_world_3d() const;
	Ref<World3D> find_world_3d() const;
#endif // _3D_DISABLED

	void set_force_viewport(Viewport *p_viewport);
	Viewport *get_force_viewport() const;
};