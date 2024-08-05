clear all ; close all ; clc

tab = readtable('napeti_RZ.csv');

U = tab{3:4098,5}; %V
t = tab{3:4098,4}; %s
RZ = 5;

tmax = 2;
tmin = -tmax;
tstep = str2double(tab{7,2})*1000; 

figure('Name','napeti_RZ_lepsi.csv', ...
'Position',[100,100,800,600]);
plot(t.*1000,-U, 'LineWidth',2)
grid on
grid minor

int = trapz(t,U.*(U./RZ));
fprintf('P RZ %d \n',int);

xticks(linspace(tmin,tmax,(-tmin+tmax)/tstep+1));
xlim([tmin, tmax]);
xlabel('t [ms]')
ylabel('U [V]')