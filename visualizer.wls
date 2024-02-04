#!/usr/bin/env wolframscript
(* ::Package:: *)

PlotBoard[board_,opts_:{}]:=Module[{},
	cols={Blue,Red,Black}[[1;;Length@board]];
	Show[MapThread[ArrayPlot[SparseArray[Thread[Reverse[#1,2]+1->1],{4,4}],
		ColorRules->{1->#2,0->Transparent},opts,Mesh->All,MeshStyle->Gray]&,{board,cols}],Background->RGBColor[1,1,1,0.8]]
]


(* Import all 2296 unique positions, each representing their equivalence class *)
boards=Import[NotebookDirectory[]<>"bin/boards.txt","Table"];
boards=Partition[boards,3][[All,{1,2,3}]];
boards=Map[Partition[#,2]&,boards,{2}];
boards//Dimensions

diagrams=PlotBoard[#,ImageSize->100]&/@boards[[1;;10]];
gr=Grid[Partition[diagrams,5],Background->White]


(* Import the graph of position transitions *)
connections=Import[NotebookDirectory[]<>"bin/network.txt","Table"]+1;
edges=Thread[#[[1]]->Drop[#,1]]&/@(connections[[1;;-1]]);
nVertices=Length@DeleteDuplicates@Flatten@connections;
network=Graph[Range[nVertices],Flatten@edges];
finalPositions=Flatten@Cases[connections,{_}]
Length/@WeaklyConnectedComponents[network]
Length/@ConnectedComponents[network]


diagrams=PlotBoard[#,ImageSize->100]&/@boards[[finalPositions]];
gr=Grid@Partition[diagrams,5]


(* Export 15 winning positions *)
Export[NotebookDirectory[]<>"solutions.png",gr,Background->Transparent]
