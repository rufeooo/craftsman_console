#include <stdio.h>

int artifactPower = 0;

int
enchant(int p)
{
  if (p > 10 || p < -10)
    return artifactPower;

  artifactPower += p;
  return artifactPower;
}

int
disenchant()
{
  --artifactPower;
  return artifactPower;
}

int
multiply()
{
  artifactPower *= 2;

  return artifactPower;
}

