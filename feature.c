#include <stdio.h>

int artifactPower = 0;

int enchant() {
  ++artifactPower;
  return artifactPower;
}

int disenchant() {
  --artifactPower;
  return artifactPower;
}

int multiply() {
  artifactPower *= 2;

  return artifactPower;
}

