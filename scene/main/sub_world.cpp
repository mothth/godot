/**************************************************************************/
/*  sub_world.cpp                                                         */
/**************************************************************************/

#include "sub_world.h"

#include "scene/main/viewport.h"
#include "scene/resources/world_2d.h"
#include "core/profiling/profiling.h"

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
#ifdef TOOLS_ENABLED
			if (data.tree->get_edited_scene_root() == this) {
				current_world_3d = get_parent()->get_viewport()->find_world_3d();
				current_world_3d->_register_sub_world(this);
			} else
#endif // TOOLS_ENABLED
			current_world_3d = world_3d.is_valid() ? world_3d : get_viewport()->find_world_3d();
			if (current_world_3d.is_valid()) {
				current_world_3d->_register_sub_world(this);
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (current_world_3d.is_valid()) {
				current_world_3d->_remove_sub_world(this);
				current_world_3d = nullptr;
			}
		} break;
	};
}

#ifndef _3D_DISABLED

void SubWorld::_viewport_entered_world_3d() {
	if (world_3d.is_null() && is_inside_tree()) {
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

void SubWorld::_viewport_exited_world_3d() {
	if (world_3d.is_null() && current_world_3d.is_valid()) {
		current_world_3d->_remove_sub_world(this);
		_propagate_exit_world_3d(this);
		current_world_3d = nullptr;
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
				// v->find_world_3d()->_remove_viewport(v);
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
				// v->find_world_3d()->_register_viewport(v);
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
	
#ifdef TOOLS_ENABLED
	if (is_inside_tree() && data.tree->get_edited_scene_root() == this) {
		world_3d = p_world_3d;
		return;
	} else
#endif // TOOLS_ENABLED

	if (p_world_3d.is_valid() && p_world_3d == current_world_3d) {
		world_3d = p_world_3d;
		return;
	}

	if (is_inside_tree() && current_world_3d.is_valid()) {
		current_world_3d->_remove_sub_world(this);
		_propagate_exit_world_3d(this);
	}
	
	world_3d = p_world_3d;

	if (is_inside_tree()) {
		current_world_3d = world_3d;
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
	// Deprecated, do nothing
}

Viewport *SubWorld::get_force_viewport() const {
	return nullptr;
}