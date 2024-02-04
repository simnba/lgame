#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

using bit_t = std::uint16_t;
using bit4_t = std::uint64_t;

#define SWAP_BITS(num, i, j) (((num >> i) & 1U) != ((num >> j) & 1U) && (num ^= (1U << i) | (1U << j)), num)
//#define SWAP_BITS(num, i, j) (((num >> i) & 1u) ^ ((num >> j) & 1u) ? (num ^= ((1u << i) | (1u << j))) : num)

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
			x += (3-2*x)*fx;
			y += (3-2*y)*fy;
			ret |= 1 << (y * 4 + x);
		}
	}
	return ret;
}

bit_t transpose(bit_t b, bool t){
	if(!t) return b;
	constexpr int pairs[6][2] = {{1,4},{2,8},{3,12},{6,9},{7,13},{11,14}};
	for (auto const& [i,j] : pairs) {
		SWAP_BITS(b,i,j);
	}
	return b;
}

struct board {
	union {
		struct {
			union {
				bit_t l[2];
				struct {
					bit_t l1, l2;
				};
			};
			bit_t s;
			bit_t playerToMove;
		};
		bit4_t data = 0;
	};

	void start(){
		l1 = (bit_t)1<<5 | (bit_t)1<<9 | (bit_t)1<<13 | (bit_t)1<<14;
		l2 = (bit_t)1<<1 | (bit_t)1<<2 | (bit_t)1<<6 | (bit_t)1<<10;
		s  = (bit_t)1<<0 | (bit_t)1<<15;
		playerToMove = 0;
	}
	bool isValid() const {
		bit_t all = l1 | l2 | s;
		return std::popcount(l1) == 4
		&& std::popcount(l2) == 4
		&& std::popcount(s) == 2
		&& std::popcount(all) == 10;
	}
	void toFile(fs::path const& filename) const {
		std::ofstream file(filename);
		for(bit_t const& s : {l1,l2,s}) {
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
struct boardHasher{
	std::size_t operator()(board const& b) const {
		return b.data;
		return std::hash<bit4_t>()(b.data);
	}
};

bool operator == (board const& b1, board const& b2){
	return b1.data == b2.data;
}

#include <vector>
#include <array>
#include <unordered_map>
#include <bit>

struct solver{
	static constexpr int nLPositions = 3*2*2*2*2;
	static constexpr int nSPositions = 16;
	std::array<bit_t,nLPositions> lPositions;
	std::array<bit_t,nSPositions> sPositions;

	solver() {
		// generate all 48 positions for an L
		bit_t p0 = (bit_t) 1 << 0 | (bit_t) 1 << 4 | (bit_t) 1 << 8 | (bit_t) 1 << 9;
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

		// the 16 positions for a single piece
		for (int t = 0; t < 16; ++t) {
			sPositions[t] = (bit_t)1<<t;
		}
	}
	void generateFollowups(board const& b, std::vector<board>& out) const {
		for (int i = 0; i < nLPositions; ++i) {
			if (lPositions[i] != b.l[b.playerToMove] && !(lPositions[i] & (b.l[1 - b.playerToMove] | b.s))) {
				board b2 = b;
				b2.l[b.playerToMove] = lPositions[i];
				b2.playerToMove = 1-b2.playerToMove;

				for (int u = 0; u < nSPositions; ++u) {
					for (int v = u+1; v < nSPositions; ++v) {
						bit_t s = sPositions[u] | sPositions[v];
						if(s & b.s) { // if one or zero stones are moved
							if (!(s & (b2.l1 | b2.l2))) { // if not collision with Ls
								b2.s = s;
								out.push_back(b2);
							}
						}
					}
				}
			}
		}
	}
	void generateEquivalents(board const& b, std::vector<board>& out) const {
		for (int i = 0, t = 0; t < 2; ++t) {
			for (int fy = 0; fy < 2; ++fy) {
				for (int fx = 0; fx < 2; ++fx) {
					board& b2 = out[i++];
					b2.l1 = transpose(flip(b.l1, fx, fy), t);
					b2.l2 = transpose(flip(b.l2, fx, fy), t);
					b2.s  = transpose(flip(b.s,  fx, fy), t);
					b2.playerToMove = b.playerToMove;
				}
			}
		}
	}

	board getNormalForm(board  b) const {
		if(b.playerToMove) {
			std::swap(b.l1, b.l2);
			b.playerToMove = 0;
		}

		boardHasher h;
		std::vector<board> variants(8);
		generateEquivalents(b, variants);
		board bn = *std::min_element(variants.begin(), variants.end(),
		                             [&h](board const& l, board const& r) {
			                             return h(l) < h(r);
		                             });
		return bn;
	}
	void generateGraph(fs::path const& boardsFilename, fs::path const& networkFilename) const {
		std::vector<board> bs;
		std::unordered_map<board, int, boardHasher> visitedIDs;

		std::vector<board> followups;
		board b;

		std::ofstream networkFile(networkFilename);


		int nValid = 0, nRejected = 0;
		for(int i = 0; i < nLPositions; ++i) {
			for(int j = 0; j < nLPositions; ++j) {
				if(!(lPositions[i] & lPositions[j])) { // if Ls not colliding
					b.l1 = lPositions[i];
					b.l2 = lPositions[j];

					// Place single pieces
					for (int u = 0; u < nSPositions; ++u) {
						for (int v = u + 1; v < nSPositions; ++v) {
							bit_t s = sPositions[u] | sPositions[v];

							if (!(s & (b.l1 | b.l2))) { // if not colliding with Ls
								b.s = s;
								b.playerToMove = 0;
								nValid++;

								board nb = getNormalForm(b);

								if(visitedIDs.try_emplace(nb,bs.size()).second)
									bs.push_back(nb);

								// exploit the fact that x->y iff T(x)->T(y) for any equivalence transformation T
								if (b == nb) {
									networkFile << visitedIDs.at(nb);

									// Calculate which positions are reachable from here
									followups.clear();
									generateFollowups(b, followups);

									for (const board& f : followups) {
										board nf = getNormalForm(f);
										if(visitedIDs.try_emplace(nf,bs.size()).second)
											bs.push_back(nf);

										networkFile << " " << visitedIDs.at(nf);
									}
									networkFile << "\n";
								}
								else {
									nRejected++;
									continue;
								}

							}
						}
					}
				}
			}
		}

		networkFile.close();

		std::cout << "Accepted " << (nValid-nRejected) << "/" << nValid << "\n";
		std::cout << "Writing to file: " << bs.size() << "\n";
		toFile(bs, boardsFilename);
	}

	static void toFile(board const& b, std::ofstream& file) {
		for (bit_t const& s: {b.l1,b.l2,b.s}) {
			for (int i = 0; i < 16; ++i) {
				if (s & ((bit_t) 1 << i)) {
					file << (i % 4) << " " << (i / 4) << " ";
				}
			}
			file << "\n";
		}
	}
	static void toFile(std::vector<board> const& bs, fs::path const& filename) {
		std::ofstream file(filename);
		for (board const& b: bs) {
			toFile(b,file);
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
	s.generateGraph("boards.txt", "network.txt");
	auto t1 = std::chrono::high_resolution_clock::now();

	std::cout << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << std::endl;
	return 0;
}
