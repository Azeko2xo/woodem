% filetype: metapost
outputformat:="svg"; prologues:=3;
newinternal numeric penYScale;
newinternal numeric penExMultiple;
penYScale:=1.;
penExMultiple:=.1;

ex:=10cm;
htt=.5*ex;
oRad=.4ex;
oDeltaY=.2*ex; % move up and down
woDist=.2oRad-ooDist;
ooDist=.5ex;
odDist=ooDist;
pair dCent,eCent;
dCent:=(odDist/2,oDeltaY);
eCent:=(odDist/2+ooDist,-oDeltaY);

wYOff=-1.0oRad;
wYA=1.5htt;
wYAB=-.3htt;
wYB=.1htt;
wYBB=-htt;
wBHt=.3htt;
wBBHt=0htt;

wWdB=ooDist;
wWdA=1.2*wWdB;
wXA=wX+wWdA;
wXB=wX+wWdA+wWdB;

wX=0-odDist/2-ooDist-woDist-wWdB-wWdA;

penWd:=penExMultiple*ex;
pair barDir;
barDir:=dir(60);
pair wEndDir;
wEndDir:=dir(80);

def letterW =
	pickup pencircle scaled penWd yscaled penYScale rotated angle(barDir);
	%linejoin:=beveled;
	linejoin:=rounded;
	linecap:=rounded;
	draw (wX,wYA)--(wX,wYAB+.5*wWdA){down}..(.5*(wX+wXA),wYAB){right}..(wXA,min(wYAB+wWdA,wYB)){wEndDir};%--(wXA,wYB);
	draw (wXA,wYB)--(wXA,min(wYBB+.5*wWdB,wYB)){down}..(.5*(wXA+wXB),-htt){right}..(wXB,-htt+wBHt){wEndDir};%--(wXB,-htt-wBBHt);
	%draw (wXB,-htt+wBHt)--(wXB,-htt);
	currentpicture:=currentpicture shifted (0,wYOff);
enddef;

def lettersOO =
	draw fullcircle scaled 2oRad shifted (0-odDist/2-ooDist,oDeltaY);
	draw fullcircle scaled 2oRad shifted (0-odDist/2,-oDeltaY);
enddef;

outputtemplate:="%j.woodem.svg";

beginfig(1);
	letterW;
	addto currentpicture also currentpicture rotated 180;
	lettersOO;

	draw subpath (0,3) of fullcircle scaled 2oRad shifted dCent;
	draw subpath (5,8) of fullcircle scaled 2oRad shifted dCent;
	draw subpath (1,7) of fullcircle scaled 2oRad shifted eCent;
	pair dPt,dEnd;
	dPt=dCent+oRad*dir(135);
	dEnd=dPt+down*whatever;
	dEnd=dCent+left*whatever;
	draw dPt--dEnd;
	pair ePt;
	ePt=eCent+oRad*dir(45);
	draw ePt--eCent;
endfig;

outputtemplate:="%j.woo.svg";
beginfig(2);
	letterW;
	lettersOO;
endfig;

outputtemplate:="%j.woo-batch.svg";
beginfig(2);
	letterW;
	currentpicture:=currentpicture shifted (-.65oRad,.3oRad);
	letterW;
	lettersOO;
	for j:=0 upto 2:
		currentpicture:=currentpicture shifted (-.55oRad,-.45oRad);
		lettersOO;
	endfor;
endfig;
end;
