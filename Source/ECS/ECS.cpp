#include "ECS.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Renderer/Renderer.h"
#include "StringFormat.h"

void ECS::init(struct Renderer* r)
{
    renderer = r;
    register_default_ecs_types(world);
    // Editor queries
    {
        editor.entity_view_query =
            world.query_builder<EditorSelectableComponent>().build();
        editor.component_view_query =
            world.query_builder<flecs::Component>()
                .term(flecs::ChildOf, flecs::Flecs)
                .oper(flecs::Not)
                .self()
                .parent()
                .build();
    }

    // Rendering queries
    {
        rendering.objects.alloc = &System_Allocator;
    }

    // Serializer
    {
        world_serializer.init(System_Allocator);
        auto register_func = Delegate<void, u64, IDescriptor*>::create_lambda(
            [this](u64 id, IDescriptor* desc) {
                world_serializer.register_descriptor(id, desc);
            });
        register_default_ecs_descriptors(register_func);
    }

    // System registrar
    {
        system_registrar = SystemDescriptorRegistrar{
            .world = world.m_world,
        };
    }

    // Component registrar
    {
        component_registrar.world = world.m_world;

        component_registrar.init(System_Allocator);
    }

    world.set<flecs::Rest>({});
}

flecs::entity ECS::create_entity(Str name)
{
    flecs::entity entity;
    if (name != Str::NullStr) {
        entity = world.entity(name.data);
    } else {
        CREATE_SCOPED_ARENA(System_Allocator, temp, 1024);

        bool created = false;
        u32  i       = 0;
        while (!created) {
            SAVE_ARENA(temp);

            if (i == 0) {
                name = format(temp, LIT("New Entity\0"));
            } else {
                name = format(temp, LIT("New Entity #{}\0"), i);
            }

            if (!world.lookup(name.data)) {
                entity  = world.entity(name.data);
                created = true;
            } else {
                i++;
            }
        }
    }

    entity.set<EditorSelectableComponent>({false, false});
    return entity;
}

void ECS::deinit()
{
    world_serializer.deinit();
    ecs_fini(world);
}

void ECS::run()
{
    world.progress();

    // Update render objects
    {
        // rendering.objects.empty();

        // rendering.transform_view_query.each(
        //     [this](
        //         flecs::entity        e,
        //         TransformComponent&  transform,
        //         StaticMeshComponent& mesh,
        //         MaterialComponent&   material) {
        //         // Compute transform
        //         glm::mat4 object_transform =
        //             glm::translate(glm::mat4(1.0f), transform.world_position)
        //             * glm::toMat4(transform.world_rotation) *
        //             glm::scale(glm::mat4(1.0f), transform.world_scale);

        //         if (mesh.mesh == 0) {
        //             mesh.mesh = renderer->get_mesh(mesh.name);
        //         }

        //         ASSERT(mesh.mesh);

        //         if (material.material == 0) {
        //             material.material =
        //                 renderer->material_system.find_material(material.name);
        //         }
        //         ASSERT(material.material);

        //         rendering.objects.add(RenderObject{
        //             .mesh      = mesh.mesh,
        //             .material  = material.material,
        //             .transform = object_transform,
        //         });
        //     });

        // renderer->render_objects = slice(rendering.objects);
    }
}

void ECS::defer(Delegate<void>&& delegate)
{
    world.defer([delegate]() { delegate.call(); });
}

void ECS::open_world(Str path)
{
    world.delete_with<EditorSelectableComponent>();
    world_serializer.import(world, path);
}

void ECS::save_world(Str path) { world_serializer.save(world, path); }