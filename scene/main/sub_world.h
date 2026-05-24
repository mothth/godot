/**************************************************************************/
/*  sub_world.h                                                           */
/**************************************************************************/
//
// SubWorld node used in The Story Machine. Allows containment of a World3D without creating a new viewport.
// Viewport is inferred from the associated World3D. SubWorld therefore allows its children to switch viewport without ever exiting the tree or world.
// NOTE: SubWorld was made for The Story Machine, and therefore is only intended for 3D and does not have a 2D counterpart.
// However, with 3D disabled, you can still use SubWorld as a means of switching viewports for child nodes using the `force_viewport` option.
// World2D support might be considered in the future.
#pragma once

#include "scene/main/node.h"

#ifndef _3D_DISABLED
class World3D;
#endif // _3D_DISABLED

class Viewport;

class SubWorld : public Node {
	GDCLASS(SubWorld, Node);

protected:

	friend class World3D;

#ifndef _3D_DISABLED
	Ref<World3D> world_3d;
	Ref<World3D> current_world_3d;
#endif // _3D_DISABLED

	Viewport *force_viewport = nullptr;

	static void _bind_methods();
	void _notification(int p_what);

	void _change_viewport(Viewport *p_viewport);
	void _update_viewport();
	void _viewport_exited();

	void _propagate_change_viewport(Node *p_node, Viewport *p_viewport);

#ifndef _3D_DISABLED
	Viewport *_preferred_viewport(Viewport *p_a, Viewport *p_b);
	Viewport *_find_world_3d_viewport();
	void _viewport_changed_world_3d();
	void _propagate_exit_world_3d(Node *p_node);
	void _propagate_enter_world_3d(Node *p_node);

	// Notifier for World3D to use
	void _new_available_viewport(Viewport *p_viewport);
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