map=load('/tmp/map.dat');
map(map(:,5)<-50,5)=nan;
x=-10:.05:10;
y=-10:.05:10;
[X,Y]=meshgrid(x,y);
a=griddata(map(:,3),map(:,4),map(:,5),X,Y);
a(a<-50)=nan;

xma=(x>=-3)&(x<=3);
yma=(y>=-3)&(y<=3);
asm=a(xma,yma);

hold off
pcolor(x(xma),y(yma),asm); shading('flat')
axis('square');

cc=[  0  60   0;
      0 120   0;
     60 160   0;
    180 180  60;
    180 180 180;
    255 255 255]/255;
ce=[0; 1000; 1500; 2500; 5000; 8000];
cce=0:8000;
ccc=[interp1(ce,cc(:,1),cce)' interp1(ce,cc(:,2),cce)' interp1(ce,cc(:,3),cce)'];
colormap(ccc)

caxis([cce(1) cce(length(cce))])

cl1=0:10:4000;
cl1=cl1(mod(cl1,100) ~= 0);
hold on; 
[c1,h1]=contour(x(xma),y(yma),asm,cl1,'k'); 
set(h1,'LineWidth',0.2);

cl2=0:100:4000;
hold on; 
[c2,h2]=contour(x(xma),y(yma),asm,cl2,'k'); 
set(h2,'LineWidth',1.0);

clabel(c2,h2,'fontsize',6)

i=0:360;
XX=-2.5+0.1*cos(i/180*pi);
YY=+2.5+0.1*sin(i/180*pi);
set(line(XX,YY),'LineWidth',2,'Color','r')

set(gcf,'PaperPosition',[.5 .5 7.5 10]) 
set(gca,'Position',[.025 .025 .95 .95])
