#!/usr/bin/env wolframscript
(* ::Package:: *)

PlotBoard[board_,opts_:{}]:=Module[{},
	cols={Blue,Red,Black}[[1;;Length@board]];
	Show[MapThread[ArrayPlot[SparseArray[Thread[Reverse[#1,2]+1->1],{4,4}],
		ColorRules->{0->Transparent,1->#2},opts,Mesh->All]&,{board,cols}]]
]


(* Import all 2296 unique positions, each representing their equivalence class *)
boards=Import[NotebookDirectory[]<>"bin/boards.txt","Table"];
boards=Partition[boards,3][[All,{1,2,3}]];
boards=Map[Partition[#,2]&,boards,{2}];
boards//Dimensions

diagrams=PlotBoard[#,ImageSize->100]&/@boards[[1;;10]];
gr=Grid@Partition[diagrams,5]


(* Import the graph of position transitions *)
connections=Import[NotebookDirectory[]<>"bin/network.txt","Table"]+1;
edges=Thread[#[[1]]->Drop[#,1]]&/@(connections[[1;;-1]]);
nVertices=Length@DeleteDuplicates@Flatten@connections;
network=Graph[Range[nVertices],DeleteDuplicates@Flatten@edges];
finalPositions=DeleteDuplicates@Flatten@Cases[connections,{_}]
cs=WeaklyConnectedComponents[network];
Length/@cs
cs=ConnectedComponents[network];
Length/@cs


diagrams=PlotBoard[#,ImageSize->100]&/@boards[[finalPositions]];
gr=Grid@Partition[diagrams,5]


(* Export 15 winning positions *)
Export[NotebookDirectory[]<>"solutions.png",gr,Background->Transparent]
