function [B,P] = civka_f(N,ls,ti,rv)
%                                              |---d--|
% N - zavity                       %  o o o| . |o o o |       
% ls [mm]- delka civky             %  o o o| . |o o o |ls
ti= ti*10^-3;%[ms] doba impulzu    %  o o o| . |o o o |
% rsl - [mm] vnitrni poloměr civky %  <-rs2->.       
% rv -  [mm] poloměr vodiče        %   rs1>--.< 
% x -  čas pro tvorbu grafu v civka_cal.m   
dv = rv*2;
u=1.25664*10^-6; % permeabilita
f=8; % Hz
U=50; % V
rs1=16; %mm
rs2=(N/(ls/dv)*dv+rs1); % mm

%% R
p = 0.018*10^-6;
lv=(2*pi*(rs2/2)*N)/1000; % m
S = (pi*(rv*10^-3)^2); % m2

R = p*(lv/S); % ohm
%% L
d=rs2-rs1; %mm
r=d/2+rs1; %mm
L=((r^2*N^2)/(19*r+29*ls+32*d))/10000000; % H
%% I
Im=U/R; % A max možny proud
I = Im*(1-exp(-ti/(L/R))); % A proud na konci impulzu
%%

B = u*(N/(ls*10^-3))*I;  % T 
P = I.*U*f.*ti;    % W 

end
