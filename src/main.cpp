#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using bit_t = std::uint16_t;
bit_t move(bit_t b, int dx, int dy){
	bit_t ret=0;
	for (int i = 0; i < 16; ++i) {
		if(b & (bit_t)1<<i) {
			auto x = (i % 4);
			auto y = (i / 4);
			x += dx;
			y += dy;
			if (0 <= x && x < 4 && 0 <= y && y < 4)
				ret |= 1u << (y * 4 + x);
		}
	}
	return ret;
}
bit_t flip(bit_t b, bool fx, bool fy){
	bit_t ret=0;
	for (int i = 0; i < 16; ++i) {
		if(b & (bit_t)1<<i) {
			int x = (i % 4);
			int y = (i / 4);
			if (fx) x = 3 - x;
			if (fy) y = 3 - y;
			ret |= 1 << (y * 4 + x);
		}
	}
	return ret;
}
bit_t transpose(bit_t b, bool t){
	if(!t) return b;
	bit_t ret=0;
	for (int i = 0; i < 16; ++i) {
		if (b & (bit_t) 1 << i) {
			auto x = (i % 4);
			auto y = (i / 4);
			ret |= 1 << (x * 4 + y);
		}
	}
	return ret;
}


struct board
{
	bit_t l1, l2, s1, s2;
	void start(){
		l1 = (bit_t)1<<5 | (bit_t)1<<9 | (bit_t)1<<13 | (bit_t)1<<14;
		l2 = (bit_t)1<<1 | (bit_t)1<<2 | (bit_t)1<<6 | (bit_t)1<<10;
		s1=(bit_t)1<<0;
		s2=(bit_t)1<<15;
	}
	bool isValid() const {
		bit_t all = l1 | l2 | s1 | s2;
		return std::popcount(l1) == 4
		&& std::popcount(l2) == 4
		&& std::popcount(s1) == 1
		&& std::popcount(s2) == 1
		&& std::popcount(all) == 10;
	}
	void toFile(fs::path const& filename) const {
		std::ofstream file(filename);
		for(bit_t const& s : {l1,l2,s1,s2}) {
			for (int i = 0; i < 16; ++i) {
				if(s & ((bit_t)1<<i)){
					file << (i%4) << " " << (i/4) << " ";
				}
			}
			file << "\n";
		}
		file.close();
	}
};

#include <vector>
struct solver{
	std::vector<bit_t> lPositions;
	std::vector<bit_t> sPositions;

	solver() {
		// generate all 48 positions for an L
		bit_t p0 = (bit_t) 1 << 0 | (bit_t) 1 << 4 | (bit_t) 1 << 8 | (bit_t) 1 << 9;
		lPositions.resize(3 * 2 * 2 * 2 * 2);
		int i = 0;
		for (int t = 0; t < 2; ++t) {
			for (int fy = 0; fy < 2; ++fy) {
				for (int fx = 0; fx < 2; ++fx) {
					for (int dy = 0; dy < 2; ++dy) {
						for (int dx = 0; dx < 3; ++dx) {
							lPositions[i++] = transpose(flip(move(p0, dx, dy), fx, fy), t);
						}
					}
				}
			}
		}

		// the 16 positions for a dot
		sPositions.resize(16);
		for (int t = 0; t < 16; ++t) {
			sPositions[t] = (bit_t)1<<t;
		}
	}

	void check(fs::path const& filename) const {
		std::vector<bit_t> ps;
		for(int i = 0; i< 48;++i){
			for(int j = 0; j< 48;++j){
				if((lPositions[i] & lPositions[j]) == 0){
					ps.push_back(lPositions[i]);
					ps.push_back(lPositions[j]);
				}
			}
		}
		toFile(ps,filename);
	}

	static void toFile(std::vector<bit_t> const& ps, fs::path const& filename) {
		std::ofstream file(filename);
		for (bit_t const& s: ps) {
			for (int i = 0; i < 16; ++i) {
				if (s & ((bit_t) 1 << i)) {
					file << (i % 4) << " " << (i / 4) << " ";
				}
			}
			file << "\n";
		}
		file.close();
	}

};

int main() {
	board b;
	b.start();
	std::cout << "valid = " << b.isValid()<<"\n";
	b.toFile("board.txt");

	solver s;
	s.check("solver.txt");

	std::cout << "Done!" << std::endl;
	return 0;
}
