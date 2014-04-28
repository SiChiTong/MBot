clear all

expdata1 = load('Exp1.txt');
expdata2 = load('Exp2.txt');


[row1,col1]=size(expdata1);
[row2,col2]=size(expdata2);


subplot(2,1,1)
plot(expdata1(5:row1,2),expdata1(5:row1,3),'r-')
hold on
plot(expdata1(5:row1,5),expdata1(5:row1,6))

subplot(2,1,2)
plot(expdata2(5:row2,2),expdata2(5:row2,3),'r-')
hold on
plot(expdata2(5:row2,5),expdata2(5:row2,6))

%%
clear all

ekfdata1 = load('EKF1.txt');
ekfdata2 = load('EKF2.txt');


[row1,col1]=size(ekfdata1);
[row2,col2]=size(ekfdata2);


subplot(2,1,1)
plot(ekfdata1(5:row1,2),ekfdata1(5:row1,3),'r-')
hold on
plot(ekfdata1(5:row1,5),ekfdata1(5:row1,6))

subplot(2,1,2)
plot(ekfdata2(5:row2,2),ekfdata2(5:row2,3),'r-')
hold on
plot(ekfdata2(5:row2,5),ekfdata2(5:row2,6))

