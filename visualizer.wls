#!/usr/bin/env wolframscript
(* ::Package:: *)

 Length \.08[{{{1,1},{1,2},{1,3},{2,3}},{{1,0},{2,0},{2,1},{2,2}},{{0,0}},{{3,3}}}]


board=Import[NotebookDirectory[]<>"bin/board.txt","Table"];
board=Partition[#,2]&/@board;
Column@board

Clear@PlotBoard;
PlotBoard[board_,opts_:{}]:=Module[{},
	cols={Blue,Red,Black,Black}[[1;;Length@board]];
	Show[MapThread[ArrayPlot[SparseArray[Thread[Reverse[#1,2]+1->1],{4,4}],
		ColorRules->{0->Transparent,1->#2},opts,Mesh->All]&,{board,cols}]]
]
PlotBoard[board,ImageSize->200]


boards=Import[NotebookDirectory[]<>"bin/solver.txt","Table"];
boards=Partition[#,2]&/@boards;
boards=Partition[boards,2];
boards//Dimensions


diagrams=PlotBoard[#]&/@boards[[1;;50]]
Grid@Partition[diagrams,6]



