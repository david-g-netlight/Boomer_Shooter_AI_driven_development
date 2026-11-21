#pragma once
#include "Registry.h"
#include <string>
#include <vector>

namespace eng::ecs
{
	/// <summary>
	/// Checks the storage rules on a small private Registry: stale ids, packed pools after removes,
	/// Each only visiting full matches, and Clear keeping its capacity. Returns the failed checks; empty means all passed.
	/// Debug only: called at startup and by the console command ecs_selftest.
	/// </summary>
	inline std::vector<std::string> RunRegistrySelfTest()
	{
		struct TestValue	{ int value = 0; };
		struct TestOther	{ int value = 0; };

		std::vector<std::string> failures;
		auto check = [&](bool ok, const char* what)
		{
			if (!ok)
				failures.push_back(what);
		};

		// Stale id: a destroyed slot that is taken again must reject the old id
		{
			Registry registry(4);
			registry.RegisterComponent<TestValue>();
			Entity first = registry.Create();
			registry.Add<TestValue>(first, TestValue{ 1 });
			registry.Destroy(first);
			Entity second = registry.Create();
			check(second.index == first.index && second.generation != first.generation, "slot reused with a new generation");
			check(!registry.IsAlive(first), "stale id is not alive");
			check(registry.Get<TestValue>(first) == nullptr, "stale id has no component");
			check(registry.Get<TestValue>(second) == nullptr, "new entity starts without the old component");
		}

		// Packed after removes: the rest keep their own values
		{
			Registry registry(8);
			registry.RegisterComponent<TestValue>();
			Entity entities[5];
			for (int i = 0; i < 5; ++i)
			{
				entities[i] = registry.Create();
				registry.Add<TestValue>(entities[i], TestValue{ i + 1 });
			}
			registry.Remove<TestValue>(entities[1]);
			check(registry.Pool<TestValue>().Size() == 4, "size after one remove");
			check(registry.Get<TestValue>(entities[1]) == nullptr, "removed component is gone");

			bool valuesKept = true;
			for (int i : { 0, 2, 3, 4 })
			{
				TestValue* value = registry.Get<TestValue>(entities[i]);
				valuesKept = valuesKept && value && value->value == i + 1;
			}
			check(valuesKept, "remaining components keep their values after a swap-remove");

			registry.Remove<TestValue>(entities[4]);
			registry.Remove<TestValue>(entities[0]);
			check(registry.Pool<TestValue>().Size() == 2, "size after three removes");
		}

		// Each: only entities with every listed component
		{
			Registry registry(8);
			registry.RegisterComponent<TestValue>();
			registry.RegisterComponent<TestOther>();
			Entity both = registry.Create();
			Entity onlyValue = registry.Create();
			Entity onlyOther = registry.Create();
			registry.Add<TestValue>(both);
			registry.Add<TestOther>(both);
			registry.Add<TestValue>(onlyValue);
			registry.Add<TestOther>(onlyOther);

			int visited = 0;
			Entity visitedEntity = INVALID_ENTITY;
			registry.Each<TestValue, TestOther>([&](Entity e, TestValue&, TestOther&)
			{
				++visited;
				visitedEntity = e;
			});
			check(visited == 1 && visitedEntity == both, "Each visits only the entities that have every component");
		}

		// Clear: everything gone, old ids stale, full capacity usable again
		{
			Registry registry(4);
			registry.RegisterComponent<TestValue>();
			Entity old[4];
			for (int i = 0; i < 4; ++i)
			{
				old[i] = registry.Create();
				registry.Add<TestValue>(old[i], TestValue{ i });
			}
			check(registry.Create() == INVALID_ENTITY, "a full registry returns an invalid entity");

			registry.Clear();
			check(registry.GetAliveCount() == 0, "Clear destroys every entity");
			check(registry.GetCapacity() == 4, "Clear keeps the capacity");
			check(!registry.IsAlive(old[0]), "ids from before Clear are stale");

			int created = 0;
			for (int i = 0; i < 4; ++i)
			{
				if (registry.Create() != INVALID_ENTITY)
					++created;
			}
			check(created == 4, "full capacity is usable after Clear");
		}

		return failures;
	}
}
