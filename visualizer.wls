#!/usr/bin/env wolframscript
(* ::Package:: *)

board=Import[NotebookDirectory[]<>"bin/board.txt","Table"];
board=Partition[#,2]&/@board;
Column@board

cols={Blue,Red,Black,Black};
opts={Mesh->All};
Show[MapThread[ArrayPlot[SparseArray[Thread[Reverse[#1,2]+1->1],{4,4}],
	ColorRules->{0->Transparent,1->#2},opts]&,{board,cols}]]



