#pragma once


// implement thinning templates given by
// Palagyi and Kuba in "A Parallel 3D 12-Subiteration Thinning Algorithm", 1999
//
#include <iostream>
#include <cstdio>

namespace pk
{

enum SpecialValues
{
  OBJECT = 1,
  D_BORDER = 10,
  SIMPLE = 20
};

enum Direction
{
  UP_SOUTH = 0,
  NORT_EAST,
  DOWN_WEST,

  SOUTH_EAST,
  UP_WEST,
  DOWN_NORTH,

  SOUTH_WEST,
  UP_NORTH,
  DOWN_EAST,

  NORT_WEST,
  UP_EAST,
  DOWN_SOUTH,

  UP,
  DOWN,
  EAST,
  WEST,
  NORTH,
  SOUTH
};

bool
matchesATemplate(unsigned char n[3][3][3])
{
  // T1
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT)) &&
      ((n[0][0][2] + n[1][0][2] + n[2][0][2] + n[0][1][2] + n[1][1][2] + n[2][1][2] + n[0][2][2] + n[1][2][2] +
        n[2][2][2]) == 0) &&
      ((n[0][0][0] + n[1][0][0] + n[2][0][0] + n[0][1][0] + n[2][1][0] + n[0][2][0] + n[1][2][0] + n[2][2][0] +
        n[0][0][1] + n[1][0][1] + n[2][0][1] + n[0][1][1] + n[2][1][1] + n[0][2][1] + n[1][2][1] + n[2][2][1]) > 0))
  {
    // it matches T1
    return true;
  }

  // T2
  if (((n[1][1][1] == OBJECT) && (n[1][2][1] == OBJECT)) &&
      (n[0][0][0] + n[1][0][0] + n[2][0][0] + n[0][0][1] + n[1][0][1] + n[2][0][1] + n[0][0][2] + n[1][0][2] +
         n[2][0][2] ==
       0) &&
      (n[0][1][0] + n[1][1][0] + n[2][1][0] + n[0][1][1] + n[2][1][1] + n[0][1][2] + n[1][1][2] + n[2][1][2] +
         n[0][2][0] + n[1][2][0] + n[2][2][0] + n[0][2][1] + n[2][2][1] + n[0][2][2] + n[1][2][2] + n[2][2][2] >
       0))
  {
    // it matches T2
    return true;
  }

  // T3
  if (((n[1][1][1] == OBJECT) && (n[1][2][0] == OBJECT)) &&
      ((n[0][0][0] + n[1][0][0] + n[2][0][0] + n[0][0][1] + n[1][0][1] + n[2][0][1] + n[0][0][2] + n[1][0][2] +
        n[2][0][2] + n[0][1][2] + n[1][1][2] + n[2][1][2] + n[0][2][2] + n[1][2][2] + n[2][2][2]) == 0) &&
      ((n[0][1][0] + n[0][2][0] + n[2][1][0] + n[2][2][0] + n[0][1][1] + n[0][2][1] + n[2][1][1] + n[2][2][1]) > 0))
  {
    // it matches T3
    return true;
  }

  // T4
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[1][2][1] == OBJECT)) &&
      ((n[1][0][1] + n[0][0][2] + n[1][0][2] + n[2][0][2] + n[1][1][2]) == 0) && ((n[0][0][1] * n[0][1][2]) == 0) &&
      ((n[2][0][1] * n[2][1][2]) == 0))
  {
    // it matches T4
    return true;
  }

  // T5
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[1][2][1] == OBJECT) && (n[2][0][2] == OBJECT)) &&
      ((n[1][0][1] + n[0][0][2] + n[1][0][2] + n[1][1][2]) == 0) && ((n[0][0][1] * n[0][1][2]) == 0) &&
      ((n[2][0][1] + n[2][1][2]) == OBJECT))
  {
    // matches T5
    return true;
  }

  // T6
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[1][2][1] == OBJECT) && (n[0][0][2] == OBJECT)) &&
      ((n[1][0][1] + n[1][0][2] + n[2][0][2] + n[1][1][2]) == 0) && ((n[0][0][1] + n[0][1][2]) == OBJECT) &&
      ((n[2][0][1] * n[2][1][2]) == 0))
  {
    // matches T6
    return true;
  }

  // T7
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[1][2][1] == OBJECT) && (n[2][1][1] == OBJECT)) &&
      ((n[1][0][1] + n[0][0][2] + n[1][0][2] + n[1][1][2]) == 0) && ((n[0][0][1] * n[0][1][2]) == 0))
  {
    return true;
  }

  // T8
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[1][2][1] == OBJECT) && (n[0][1][1] == OBJECT)) &&
      ((n[1][0][1] + n[1][0][2] + n[2][0][2] + n[1][1][2]) == 0) && ((n[2][0][1] * n[2][1][2]) == 0))
  {
    return true;
  }

  // T9
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[1][2][1] == OBJECT) && (n[2][1][1] == OBJECT) &&
       (n[0][0][2] == OBJECT)) &&
      ((n[1][0][1] + n[1][0][2] + n[1][1][2]) == 0) && ((n[0][0][1] + n[0][1][2]) == OBJECT))
  {
    return true;
  }

  // T10
  if (((n[1][1][1] == OBJECT) && (n[1][1][0] == OBJECT) && (n[0][1][1] == OBJECT) && (n[1][2][1] == OBJECT) &&
       (n[2][0][2] == OBJECT)) &&
      ((n[1][0][1] + n[1][0][2] + n[1][1][2]) == 0) && ((n[2][0][1] + n[2][1][2]) == OBJECT))
  {
    return true;
  }

  // T11
  if (((n[1][1][1] == OBJECT) && (n[2][1][0] == OBJECT) && (n[1][2][0] == OBJECT)) &&
      ((n[0][0][0] + n[1][0][0] + n[0][0][1] + n[1][0][1] + n[0][0][2] + n[1][0][2] + n[2][0][2] + n[0][1][2] +
        n[1][1][2] + n[2][1][2] + n[0][2][2] + n[1][2][2] + n[2][2][2]) == 0))
  {
    return true;
  }

  // T12
  if (((n[1][1][1] == OBJECT) && (n[0][1][0] == OBJECT) && (n[1][2][0] == OBJECT)) &&
      ((n[1][0][0] + n[2][0][0] + n[1][0][1] + n[2][0][1] + n[0][0][2] + n[1][0][2] + n[2][0][2] + n[0][1][2] +
        n[1][1][2] + n[2][1][2] + n[0][2][2] + n[1][2][2] + n[2][2][2]) == 0))
  {
    return true;
  }

  // T13
  if (((n[1][1][1] == OBJECT) && (n[1][2][0] == OBJECT) && (n[2][2][1] == OBJECT)) &&
      ((n[0][0][0] + n[1][0][0] + n[2][0][0] + n[0][0][1] + n[1][0][1] + n[2][0][1] + n[0][0][2] + n[1][0][2] +
        n[2][0][2] + n[0][1][2] + n[1][1][2] + n[0][2][2] + n[1][2][2]) == 0))
  {
    return true;
  }

  // T14
  if (((n[1][1][1] == OBJECT) && (n[1][2][0] == OBJECT) && (n[0][2][1] == OBJECT)) &&
      ((n[0][0][0] + n[1][0][0] + n[2][0][0] + n[0][0][1] + n[1][0][1] + n[2][0][1] + n[0][0][2] + n[1][0][2] +
        n[2][0][2] + n[1][1][2] + n[2][1][2] + n[1][2][2] + n[2][2][2]) == 0))
  {
    return true;
  }

  return false;
}

// transform neighborhood from a different direction
void
transformNeighborhood(const unsigned char n[3][3][3], char direction, unsigned char USn[3][3][3])
{
  short         i, j, k;
  unsigned char tmp[3][3][3];

  switch (direction)
  {
    case 0: // UP_SOUTH = 0,
      // just copy
      memcpy(USn, n, 27 * sizeof(unsigned char));
      break;
    case 1: // NORT_EAST,
      // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            tmp[j][2 - i][k] = n[i][j][k];
          }
        }
      }
      // 2
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[2 - k][j][i] = tmp[i][j][k];
          }
        }
      }
      break;
    case 2: // DOWN_WEST,
            // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            tmp[2 - j][i][k] = n[i][j][k];
          }
        }
      }
      // 2
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[i][j][2 - k] = tmp[i][j][k];
          }
        }
      }
      break;
    case 3: // SOUTH_EAST,
            // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[2 - k][j][i] = n[i][j][k];
          }
        }
      }
      break;
    case 4: // UP_WEST,
      // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[2 - j][i][k] = n[i][j][k];
          }
        }
      }
      break;
    case 5: // DOWN_NORTH,
            // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            tmp[i][2 - j][k] = n[i][j][k];
          }
        }
      }
      // 2
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[i][j][2 - k] = tmp[i][j][k];
          }
        }
      }
      break;
    case 6: // SOUTH_WEST,
      // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[k][j][2 - i] = n[i][j][k];
          }
        }
      }
      break;
    case 7: // UP_NORTH,
      // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[i][2 - j][k] = n[i][j][k];
          }
        }
      }
      break;
    case 8: // DOWN_EAST,
            // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            tmp[j][2 - i][k] = n[i][j][k];
          }
        }
      }
      // 2
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[i][j][2 - k] = tmp[i][j][k];
          }
        }
      }
      break;
    case 9: // NORT_WEST,
            // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            tmp[2 - j][i][k] = n[i][j][k];
          }
        }
      }
      // 2
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[k][j][2 - i] = tmp[i][j][k];
          }
        }
      }
      break;
    case 10: // UP_EAST,
             // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[j][2 - i][k] = n[i][j][k];
          }
        }
      }
      break;
    case 11: //  DOWN_SOUTH,
             // 1
      for (k = 0; k < 3; k++)
      {
        for (j = 0; j < 3; j++)
        {
          for (i = 0; i < 3; i++)
          {
            USn[i][j][2 - k] = n[i][j][k];
          }
        }
      }
      break;
  }
}

template <typename TIndex>
void
markBoundaryInDirection(unsigned char * vol, TIndex L, TIndex M, TIndex N, TIndex offset)
{
  TIndex idx;
  TIndex i, j, k;

  for (k = 1; k < (N - 1); k++)
  {
    for (j = 1; j < (M - 1); j++)
    {
      for (i = 1; i < (L - 1); i++)
      {
        idx = (k * M + j) * L + i;
        if (vol[idx] == OBJECT && vol[idx + offset] == 0)
        {
          vol[idx] = D_BORDER;
        }
      }
    }
  }
}

template <typename TIndex>
void
copyNeighborhoodInBuffer(const unsigned char * vol,
                         const TIndex          idx,
                         const TIndex          volNeighbors[27],
                         unsigned char         nb[3][3][3])
{
  short i, j, k, ii;

  ii = 0;
  for (k = 0; k < 3; ++k)
  {
    for (j = 0; j < 3; ++j)
    {
      for (i = 0; i < 3; ++i, ++ii)
      {
        nb[i][j][k] = (vol[idx + volNeighbors[ii]] != 0) ? OBJECT : 0;
      }
    }
  }
}

} // namespace pk