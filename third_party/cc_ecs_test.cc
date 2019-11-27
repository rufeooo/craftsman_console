#include <iostream>
#include "submodule/test_games/ecs/ecs.h"

struct VelocityComponent {
  VelocityComponent() = default;
  VelocityComponent(float dx, float dy) : dx_(dx), dy_(dy) {}

  float dx_;
  float dy_;
};

static ecs::ComponentStorage<VelocityComponent> globalEcs;

size_t entityCount;

extern "C" {

size_t spawn(size_t testSize) {
  if (testSize) {
    globalEcs.Clear<VelocityComponent>();
    entityCount = testSize;
  }
  for (int i = 0; i < entityCount; ++i) {
    globalEcs.Assign<VelocityComponent>(i, 2.0, 3.0);
  }
  return entityCount;
}

size_t getEntities() {
  for (int i = 0; i < entityCount; ++i) {
    VelocityComponent *c = globalEcs.Get<VelocityComponent>(i);
  }

  return entityCount;
}

size_t enumerateEntities() {
  int count = 0;
  globalEcs.Enumerate<VelocityComponent>(
      [&count](ecs::Entity ent, auto &c) { ++count; });
  return count;
}

size_t destroyEntities() {
  for (int i = 0; i < entityCount; ++i) {
    globalEcs.Remove<VelocityComponent>(i);
  }

  return entityCount;
}

size_t speak() {
  std::cout << "Entity count: " << entityCount << std::endl;
  return 0;
}
}

