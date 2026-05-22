/**************************************************************************/
/*  sub_world.cpp                                                         */
/**************************************************************************/

// Currently a bit spaghetti. Oh well.
#include "sub_world.h"

#include "scene/main/viewport.h"
#include "scene/resources/world_2d.h"

#ifndef _3D_DISABLED
#include "scene/resources/3d/world_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/3d/node_3d.h"
#endif // _3D_DISABLED

void SubWorld::_bind_methods() {
#ifndef _3D_DISABLED

	ClassDB::bind_method(D_METHOD("set_world_3d", "world_3d"), &SubWorld::set_world_3d);
	ClassDB::bind_method(D_METHOD("get_world_3d"), &SubWorld::get_world_3d);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_3d", PROPERTY_HINT_RESOURCE_TYPE, "World3D"), "set_world_3d", "get_world_3d");

#endif // _3D_DISABLED

	ClassDB::bind_method(D_METHOD("set_force_viewport", "viewport"), &SubWorld::set_force_viewport);
	ClassDB::bind_method(D_METHOD("get_force_viewport"), &SubWorld::get_force_viewport);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "force_viewport", PROPERTY_HINT_NODE_TYPE, "Viewport"), "set_force_viewport", "get_force_viewport");

	ADD_SIGNAL(MethodInfo("viewport_changed"));
}

void SubWorld::_notification(int p_what) {
	ERR_MAIN_THREAD_GUARD;

	switch (p_what) {

		case NOTIFICATION_ENTER_TREE: {
			data.viewport = nullptr;

			if (world_3d.is_valid()) {
				current_world_3d = world_3d;
				current_world_3d->_register_sub_world(this);
			}

			_update_viewport();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (current_world_3d.is_valid()) {
				current_world_3d->_remove_sub_world(this);
				current_world_3d = nullptr;
			}

			if (get_viewport()) {
				_change_viewport(nullptr);
			}
		} break;
	};
}

void SubWorld::_change_viewport(Viewport *p_viewport) {
	if (data.viewport) {
#ifndef _3D_DISABLED
		data.viewport->disconnect(SNAME("world_3d_changed"), callable_mp(this, &SubWorld::_viewport_changed_world_3d));
#endif // _3D_DISABLED
		data.viewport->disconnect(SNAME("tree_exiting"), callable_mp(this, &SubWorld::_viewport_changed_world_3d));

		propagate_notification_in_tree(NOTIFICATION_EXIT_VIEWPORT);
	}

	_propagate_change_viewport(this, p_viewport);
	emit_signal(SNAME("viewport_changed"));

	if (p_viewport) {
#ifndef _3D_DISABLED
		data.viewport->connect(SNAME("world_3d_changed"), callable_mp(this, &SubWorld::_viewport_changed_world_3d));
#endif // _3D_DISABLED
		data.viewport->connect(SNAME("tree_exiting"), callable_mp(this, &SubWorld::_viewport_changed_world_3d));

		propagate_notification_in_tree(NOTIFICATION_ENTER_VIEWPORT);
		if (!world_3d.is_valid()) {
			_viewport_changed_world_3d();
		}
	}
}

void SubWorld::_update_viewport() {
	Viewport *vp = nullptr;

	if (is_inside_tree()) {

		if (force_viewport) {
			vp = force_viewport;

#ifndef _3D_DISABLED
			if (world_3d.is_valid() && vp->find_world_3d() != world_3d) {
				// Should throw an error maybe?
				vp = nullptr;
			}
#endif // _3D_DISABLED

		}

#ifndef _3D_DISABLED
		else if (world_3d.is_valid()) {
			vp = _find_world_3d_viewport();
		}
#endif // _3D_DISABLED

		else if (!world_3d.is_valid() && data.parent) {
			vp = data.parent->data.viewport;
		}
	}
	
	if (vp != data.viewport) {
		_change_viewport(vp);
	}
}

void SubWorld::_propagate_change_viewport(Node *p_node, Viewport *p_viewport) {
	p_node->data.viewport = p_viewport;
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_propagate_change_viewport(p_node->get_child(i), p_viewport);
	}
}

#ifndef _3D_DISABLED

Viewport *SubWorld::_preferred_viewport(Viewport *p_a, Viewport *p_b) {
	if (!p_a) {
		return p_b;
	} else if (!p_b) {
		return p_a;
	} else if (!p_b->get_parent_viewport() && p_a->get_parent_viewport()) {
		return p_b;
	} else if (p_b->get_priority() > p_a->get_priority()) {
		return p_b;
	}

	// TODO: Check update mode to prefer a viewport that is being actively updated?

	return p_a;
}

Viewport *SubWorld::_find_world_3d_viewport() {
	if (!current_world_3d.is_valid()) {
		return nullptr;
	}

	const HashSet<Viewport *> vps = current_world_3d->get_viewports();

	Viewport *vp = nullptr;
	for (auto v : vps) {
		vp = _preferred_viewport(vp, v);
	}

	return vp;
}

void SubWorld::_new_available_viewport(Viewport *p_viewport) {
	if (!world_3d.is_valid()) {
		// We weren't bound to the World3D so we don't care about this
		return;
	}

	Viewport *vp = _preferred_viewport(get_viewport(), p_viewport);
	if (vp != get_viewport()) {
		_change_viewport(vp);
	}
}

void SubWorld::_viewport_changed_world_3d() {
	if (!is_inside_tree()) {
		_change_viewport(nullptr);
		return;
	}

	if (world_3d.is_valid()) {
		_update_viewport();
	}
	else {
		if (current_world_3d.is_valid()) {
			current_world_3d->_remove_sub_world(this);
			_propagate_exit_world_3d(this);
		}

		current_world_3d = get_viewport()->find_world_3d();

		if (current_world_3d.is_valid()) {
			current_world_3d->_register_sub_world(this);
			_propagate_enter_world_3d(this);
		}
	}
}

void SubWorld::_propagate_exit_world_3d(Node *p_node) {
	if (p_node != this) {
		// May have exited scene already
		if (!p_node->is_inside_tree()) {
			return;
		}

		if (Object::cast_to<Node3D>(p_node) || Object::cast_to<WorldEnvironment>(p_node)) {
			p_node->notification(Node3D::NOTIFICATION_EXIT_WORLD, true);
		}
		else if (Object::cast_to<SubWorld>(p_node)) {
			return;
		}
		else {
			Viewport *v = Object::cast_to<Viewport>(p_node);
			if (v) {
				if (v->get_world_3d().is_valid() || v->is_using_own_world_3d()) {
					return;
				}
			}
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_propagate_exit_world_3d(p_node->get_child(i));
	}
}

void SubWorld::_propagate_enter_world_3d(Node *p_node) {
	if (p_node != this) {
		// Node may not have entered the tree yet
		if (!p_node->is_inside_tree()) {
			return;
		}

		if (Object::cast_to<Node3D>(p_node)){
			static_cast<Node3D *>(p_node)->data.world_3d = current_world_3d;
			p_node->notification(Node3D::NOTIFICATION_ENTER_WORLD);
		}
		else if (Object::cast_to<WorldEnvironment>(p_node)) {
			static_cast<WorldEnvironment *>(p_node)->world_3d = current_world_3d;
			p_node->notification(Node3D::NOTIFICATION_ENTER_WORLD);
		}
		else if (Object::cast_to<SubWorld>(p_node)) {
			return;
		}
		else {
			Viewport *v = Object::cast_to<Viewport>(p_node);
			if (v) {
				if (v->get_world_3d().is_valid() || v->is_using_own_world_3d()) {
					return;
				}
			}
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_propagate_enter_world_3d(p_node->get_child(i));
	}
}

void SubWorld::set_world_3d(const Ref<World3D> &p_world_3d) {
	if (p_world_3d == world_3d) {
		return;
	}

	if (p_world_3d.is_valid() && p_world_3d == current_world_3d) {
		world_3d = p_world_3d;
		_update_viewport();
		return;
	}

	if (is_inside_tree() && current_world_3d.is_valid()) {
		current_world_3d->_remove_sub_world(this);
		_propagate_exit_world_3d(this);
	}
	
	world_3d = p_world_3d;

	if (is_inside_tree()) {
		current_world_3d = world_3d;
		_update_viewport();
		if (world_3d.is_valid()) {
			world_3d->_register_sub_world(this);
			_propagate_enter_world_3d(this);
		}
	}
}

Ref<World3D> SubWorld::get_world_3d() const {
	ERR_READ_THREAD_GUARD_V(Ref<World3D>());
	return world_3d;
}

Ref<World3D> SubWorld::find_world_3d() const {
	ERR_READ_THREAD_GUARD_V(Ref<World3D>());
	return current_world_3d;
}

#endif // _3D_DISABLED

void SubWorld::set_force_viewport(Viewport *p_viewport) {
	if (force_viewport != p_viewport) {
		if (p_viewport) {
			ERR_FAIL_COND_MSG(world_3d.is_valid() && p_viewport->find_world_3d() != world_3d, "Viewport does not have assigned World3D");
		}
		force_viewport = p_viewport;
		if (is_inside_tree()) {
			_update_viewport();
		}
	}
}

Viewport *SubWorld::get_force_viewport() const {
	return force_viewport;
}