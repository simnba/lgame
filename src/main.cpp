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
	bit_t l1 = 0, l2 = 0, s1 = 0, s2 = 0;
	void normalize(){


		// L-swap symmetry
		if(l1 > l2) std::swap(l1,l2);
		if(s1 > s2) std::swap(s1,s2);

		// x-symmetry
		if(flip(l1,true,false) < l1){
			l1 = flip(l1,true,false);
			l2 = flip(l2,true,false);
		}
	}
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
struct boardHasher
{
	std::size_t operator()(board const& b) const {
		std::uint64_t all =   (std::uint64_t)b.l1<<48 | (std::uint64_t)b.l2<<32
							| (std::uint64_t)b.s1<<16 | (std::uint64_t)b.s2;
		return std::hash<std::uint64_t>()(all);
	}
};

bool operator == (board const& b1, board const& b2){
	boardHasher h;
	return h(b1) == h(b2);
}

#include <vector>
#include <unordered_set>

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

	void generateEquivalents(board const& b, std::vector<board>& out) const {
		int i = 0;
		for (int t = 0; t < 2; ++t) {
			for (int fy = 0; fy < 2; ++fy) {
				for (int fx = 0; fx < 2; ++fx) {
					for (int s = 0; s < 2; ++s) {
						board b2;
						b2.l1 = transpose(flip(b.l1, fx, fy), t);
						b2.l2 = transpose(flip(b.l2, fx, fy), t);
						if(s) std::swap(b2.l1,b2.l2);
						out[i++] = b2;
					}
				}
			}
		}
	}
	void generatePositions(fs::path const& filename) const {
		std::unordered_set<board, boardHasher> visited;
		std::vector<board> bs, variants(16);
		boardHasher h;
		int nValid = 0, nRejected = 0;
		for(int i = 0; i < 48; ++i){
			for(int j = 0; j < 48; ++j){
				if((lPositions[i] & lPositions[j]) == 0){
					nValid++;
					board b;
					b.l1 = lPositions[i];
					b.l2 = lPositions[j];

					generateEquivalents(b, variants);


					board bn = *std::min_element(variants.begin(),variants.end(),[&h](board const& l, board const& r){return h(l) < h(r);});

					if(!visited.contains(bn)) {
						bs.push_back(b);
						visited.insert(bn);
					}
					else
						nRejected++;


//					if(b == bn) {
//						bs.push_back(b);
//					}
//					else
//						nRejected++;
				}
			}
		}
		std::cout << "Accepted " << (nValid-nRejected) << "/" << nValid << "\n";
		toFile(bs,filename);
	}

	static void toFile(std::vector<board> const& bs, fs::path const& filename) {
		std::ofstream file(filename);
		for (board const& b: bs) {
			for (bit_t const& s: {b.l1,b.l2,b.s1,b.s2}) {
				for (int i = 0; i < 16; ++i) {
					if (s & ((bit_t) 1 << i)) {
						file << (i % 4) << " " << (i / 4) << " ";
					}
				}
				file << "\n";
			}
		}
		file.close();
	}

};

#include <chrono>

int main() {
	board b;
	b.start();
	std::cout << "valid = " << b.isValid()<<"\n";
	b.toFile("board.txt");

	solver s;

	auto t0 = std::chrono::high_resolution_clock::now();
	s.generatePositions("solver.txt");
	auto t1 = std::chrono::high_resolution_clock::now();

	std::cout << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << std::endl;
	return 0;
}
