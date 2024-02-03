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


struct board {
	union{
		bit_t l[2] = {0,0};
		struct{
			bit_t l1, l2;
		};
	};
	bit_t s = 0;
	bit_t playerToMove = 0;

	void normalize(){
		// L-swap symmetry
		if(l1 > l2) std::swap(l1,l2);

		// x-symmetry
		if(flip(l1,true,false) < l1){
			l1 = flip(l1,true,false);
			l2 = flip(l2,true,false);
		}
	}
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
struct boardHasher
{
	std::size_t operator()(board const& b) const {
		std::uint64_t all =   (std::uint64_t)b.l1<<48 | (std::uint64_t)b.l2<<32
							 | (std::uint64_t)b.s<<16 | (std::uint64_t)b.playerToMove;
		return std::hash<std::uint64_t>()(all);
	}
};

bool operator == (board const& b1, board const& b2){
	boardHasher h;
	return h(b1) == h(b2);
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

		// the 16 positions for a dot
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
						if(s==1152)
							int sfkh = 345;
						if(std::popcount(bit_t(s & b.s))>0) { // if one or zero stones are moved
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
		int i = 0;
		for (int t = 0; t < 2; ++t) {
			for (int fy = 0; fy < 2; ++fy) {
				for (int fx = 0; fx < 2; ++fx) {
					for (int s = 0; s < 2; ++s) {
						board& b2 = out[i++];
						b2.l1 = transpose(flip(b.l1, fx, fy), t);
						b2.l2 = transpose(flip(b.l2, fx, fy), t);
						b2.s  = transpose(flip(b.s,  fx, fy), t);
						b2.playerToMove = b.playerToMove;
						if(s) {
							std::swap(b2.l1, b2.l2);
							b2.playerToMove = 1-b2.playerToMove;
						}
					}
				}
			}
		}
	}
	board getNormalForm(board const& b, bool check = true) const {
		boardHasher h;
		std::vector<board> variants(16);
		generateEquivalents(b, variants);
		board bn = *std::min_element(variants.begin(), variants.end(),
		                             [&h](board const& l, board const& r) {
			                             return h(l) < h(r);
		                             });
		if(check && bn != getNormalForm(bn,false))
			std::cout << "oh no \n";
		return bn;
	}
	void generatePositions(fs::path const& filename, fs::path const& networkFilename) const {
		std::vector<board> bs;
		std::unordered_map<board, int, boardHasher> visitedIDs;

		std::vector<board> followups;
		board b;
		boardHasher h;

		std::ofstream file(networkFilename);



		b.l1=71;
		b.l2=12560;
		b.s=136;
		b.playerToMove=1;
		followups.clear();
		generateFollowups(b, followups);


		int nValid = 0, nRejected = 0;
		for(int i = 0; i < nLPositions; ++i) {
			for(int j = 0; j < nLPositions; ++j) {
				if(!(lPositions[i] & lPositions[j])) { // if Ls not colliding
					b.l1 = lPositions[i];
					b.l2 = lPositions[j];

					// Place single pieces
					for (int u = 0; u < 16; ++u) {
						for (int v = u + 1; v < 16; ++v) {
							bit_t s = sPositions[u] | sPositions[v];
							if (!(s & (b.l1 | b.l2))) { // if not colliding with Ls
								for (int player = 0; player < 2; ++player) {
									b.s = s;
									b.playerToMove = player;
									nValid++;

									// Calculate which positions are reachable from here
									followups.clear();
									generateFollowups(b, followups);

									board nb = getNormalForm(b);

									if (!visitedIDs.contains(nb)) {
										bs.push_back(nb);
										visitedIDs[nb] = bs.size() - 1;
									}
									if(h(b)==10132284209008386232)
										int hgfh = visitedIDs.at(nb);
									file << visitedIDs.at(nb);

									for (board& f : followups) {
										board nf = getNormalForm(f);
										if (!visitedIDs.contains(nf)) {
											bs.push_back(nf);
											visitedIDs[nf] = bs.size() - 1;
										}
										file << " " << visitedIDs.at(nf);
									}
									file << "\n";


									// if this is the normal form of the board
									if (b == nb) {

									}
									else
										nRejected++;
								}
							}
						}
					}
				}
			}
		}

		file.close();



		std::cout << bs[1916].l1 << "\n";
		std::cout << bs[1916].l2 << "\n";
		std::cout << bs[1916].s << "\n";
		std::cout << bs[1916].playerToMove << "\n";


		b.l1=71;
		b.l2=12560;
		b.s=136;
		b.playerToMove=1;
		followups.clear();
		generateFollowups(b, followups);
		for(auto f : followups){
			if(visitedIDs.contains(f)){ // is f normal
				std::cout << "to normal "<< visitedIDs[f]<<"\n"; // should catch 1917
			}
			if(f==bs[1916]){ // is f is 1917
				std::cout << "found "<< visitedIDs[f]<<"\n"; // should catch 1917
			}
		}

		std::vector<board> variants(16);
		generateEquivalents(bs[1916], variants);
		for(auto& a : variants)
			if(visitedIDs.contains(a))
				std::cout << "shit "<<visitedIDs[a]<<"\n";


		std::ofstream f2 ("solver.txt");
		board pre = bs[1916];
		pre.playerToMove = 1-pre.playerToMove;
		followups.clear();
		generateFollowups(pre,followups);
		for(auto f : followups){
			f.playerToMove = 1-f.playerToMove;
			// f is a pre-state of 1916
			auto fn = getNormalForm(f);
			if(visitedIDs.contains(fn)) {
				std::cout << "prestate is hash " << h(f) << "\n";

				toFile(bs[1916],f2);
				toFile(f,f2);
				toFile(fn,f2);

				break;
			}
		}
		f2.close();

//		boardHasher h;
//		std::sort(bs.begin(), bs.end(),
//		                             [&h](board const& l, board const& r) {
//			                             return h(l) < h(r);
//		                             });
//		auto last = std::unique(bs.begin(), bs.end());
//
//		bs.erase(last,bs.end());

		std::cout << "Accepted " << (nValid-nRejected) << "/" << nValid << "\n";
		std::cout << "Writing to file: " << bs.size() << "\n";
		toFile(bs,filename);
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
	s.generatePositions("boards.txt", "network.txt");
	auto t1 = std::chrono::high_resolution_clock::now();

	std::cout << "Took " << std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() << std::endl;
	return 0;
}
