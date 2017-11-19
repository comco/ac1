// Usage: ./allcols 13 pic.ppm
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>

typedef unsigned short u16;

struct index {
  int i, j, k;

  index(int i = -1, int j = 0, int k = 0) : i{i}, j{j}, k{k} {}

  bool empty() const { return i == -1; }
};

std::ostream& operator<<(std::ostream& out, const index& id) {
  return out << '(' << id.i << ", " << id.j << ", " << id.k << ')';
}

struct cube {
  u16 data[16][16];

  cube() : data{{0}} {}

  bool empty() const {
    for (int i = 0; i < 16; ++i) {
      for (int j = 0; j < 16; ++j) {
        if (data[i][j]) return false;
      }
    }
    return true;
  }

  bool get(int i, int j, int k) const { return (data[i][j] >> k) & 1; }

  void set(int i, int j, int k) { data[i][j] |= 1 << k; }

  void clr(int i, int j, int k) { data[i][j] &= ~(1 << k); }

  void closest(int oi, int oj, int ok, int& min_len, std::vector<index>& out) {
    for (int i = 0; i < 16; ++i) {
      int di = i - oi;
      int leni = di * di;
      if (leni > min_len) {
        if (i >= oi) break;
        continue;
      }
      for (int j = 0; j < 16; ++j) {
        if (!data[i][j]) continue;
        int dj = j - oj;
        int lenj = leni + dj * dj;
        if (lenj > min_len) {
          if (j >= oj) break;
          continue;
        }
        for (int k = 0; k < 16; ++k) {
          if (get(i, j, k)) {
            int dk = k - ok;
            int len = lenj + dk * dk;
            if (len > min_len) continue;
            if (len < min_len) {
              min_len = len;
              out.clear();
            }
            out.emplace_back(index{di, dj, dk});
          }
        }
      }
    }
  }
};

std::ostream& operator<<(std::ostream& out, const cube& c) {
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) {
      for (int k = 0; k < 16; ++k) {
        out << c.get(i, j, k);
      }
      out << std::endl;
    }
  }
  return out;
}

struct cube2 {
  cube sup;
  cube sub[16][16][16];

  cube2() : sup{}, sub{} {}

  bool get(int i, int j, int k) const {
    return sub[i / 16][j / 16][k / 16].get(i % 16, j % 16, k % 16);
  }

  void set(int i, int j, int k) {
    sup.set(i / 16, j / 16, k / 16);
    sub[i / 16][j / 16][k / 16].set(i % 16, j % 16, k % 16);
  }

  void clr(int i, int j, int k) {
    sub[i / 16][j / 16][k / 16].clr(i % 16, j % 16, k % 16);
    if (sub[i / 16][j / 16][k / 16].empty()) {
      sup.clr(i / 16, j / 16, k / 16);
    }
  }

  void closest(int oi, int oj, int ok, int& min_len, std::vector<index>& out,
               int& min_len2, std::vector<index>& out2) {
    sup.closest(oi / 16, oj / 16, ok / 16, min_len, out);
    for (const index& id : out) {
      int ci = oi / 16 + id.i;
      int cj = oj / 16 + id.j;
      int ck = ok / 16 + id.k;
      int ti = oi - 16 * ci;
      int tj = oj - 16 * cj;
      int tk = ok - 16 * ck;
      sub[ci][cj][ck].closest(ti, tj, tk, min_len2, out2);
    }
  }
};

struct point {
  int x, y;
};

struct pixmap {
  cube2 c;
  index data[4096][4096];
  point pos[256][256][256];

  pixmap() : c{}, pos{} {
    for (int x = 0; x < 4096; ++x) {
      for (int y = 0; y < 4096; ++y) {
        data[x][y].i = -1;
      }
    }
  }

  static bool in(int x, int y) {
    return 0 <= x && x < 4096 && 0 <= y && y < 4096;
  }

  void set(int x, int y, const index& id) {
    data[x][y] = id;
    pos[id.i][id.j][id.k] = point{x, y};
    c.set(id.i, id.j, id.k);
  }

  const std::vector<point> dirs{{-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
                                {0, 1},   {1, -1}, {1, 0},  {1, 1}};
  /*
const std::vector<point> dirs{{-2, -1}, {-2, 1}, {2, -1}, {2, 1},
                          {-1, -2}, {-1, 2}, {1, -2}, {1, 2}};
  */

  void around(int x, int y, std::vector<point>& out) const {
    for (const point& d : dirs)
      if (in(x + d.x, y + d.y)) out.emplace_back(point{x + d.x, y + d.y});
  }

  bool boundary(int x, int y) const {
    if (!in(x, y)) return false;
    if (data[x][y].empty()) return false;
    for (const point& d : dirs) {
      if (in(x + d.x, y + d.y) && data[x + d.x][y + d.y].empty()) return true;
    }
    return false;
  }

  void add_boundaries() {
    for (int x = 0; x < 4096; ++x) {
      for (int y = 0; y < 4096; ++y) {
        if (boundary(x, y)) {
          int i = data[x][y].i;
          int j = data[x][y].j;
          int k = data[x][y].k;
          c.set(i, j, k);
        }
      }
    }
  }

  void clr_boundaries_around(int x, int y) {
    for (const point& d : dirs) {
      int nx = x + d.x;
      int ny = y + d.y;
      if (in(nx, ny) && !data[nx][ny].empty() && !boundary(nx, ny)) {
        int i = data[nx][ny].i;
        int j = data[nx][ny].j;
        int k = data[nx][ny].k;
        c.clr(i, j, k);
      }
    }
  }

  void add_color(int i, int j, int k) {
    int min_len = std::numeric_limits<int>::max();
    int min_len2 = std::numeric_limits<int>::max();
    static std::vector<index> out;
    static std::vector<index> out2;
    out.reserve(16);
    out2.reserve(16);
    out.clear();
    out2.clear();
    c.closest(i, j, k, min_len, out, min_len2, out2);
    int ni = i + out2[0].i;
    int nj = j + out2[0].j;
    int nk = k + out2[0].k;
    int px = pos[ni][nj][nk].x;
    int py = pos[ni][nj][nk].y;
    static std::vector<point> pts;
    pts.reserve(16);
    pts.clear();
    around(px, py, pts);
    std::random_shuffle(pts.begin(), pts.end());
    for (const point& pt : pts) {
      if (data[pt.x][pt.y].empty()) {
        data[pt.x][pt.y] = index{i, j, k};
        pos[i][j][k] = pt;
        if (boundary(pt.x, pt.y)) c.set(i, j, k);
        clr_boundaries_around(pt.x, pt.y);
        return;
      }
    }
  }

  void write(FILE* fp) {
    fprintf(fp, "P6\n%d %d\n255\n", 4096, 4096);
    for (int x = 0; x < 4096; ++x) {
      for (int y = 0; y < 4096; ++y) {
        static unsigned char color[3];
        color[0] = data[x][y].i;
        color[1] = data[x][y].j;
        color[2] = data[x][y].k;
        fwrite(color, 1, 3, fp);
      }
    }
  }
};

std::vector<index> all_colors() {
  std::vector<index> r;
  r.reserve(256 * 256 * 256);
  for (int i = 0; i < 256; ++i) {
    for (int j = 0; j < 256; ++j) {
      for (int k = 0; k < 256; ++k) {
        r.emplace_back(index{i, j, k});
      }
    }
  }
  return r;
}

int main(int argc, char* argv[]) {
  int seed = std::atoi(argv[1]);
  srand(seed);
  pixmap* p = new pixmap;

  std::vector<index> cols = all_colors();
  std::random_shuffle(cols.begin(), cols.end());

  size_t i = 0;
  // p->set(2048, 2048, cols[i++]);
  for (int j = 0; j < 4096; ++j) {
    p->set(0, j, cols[i++]);
  }
  for (; i < cols.size(); ++i) {
    const index& id = cols[i];
    p->add_color(id.i, id.j, id.k);
  }

  FILE* file = fopen(argv[2], "wb");
  p->write(file);
  fclose(file);
  return 0;
}
