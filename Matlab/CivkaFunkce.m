function [B,P,B2,res] = civka_f(N,ls,ti,rv,t)
%                                              |---d--|
% N - zavity                       %  o o o| . |o o o |       
% ls [mm]- delka civky             %  o o o| . |o o o |ls
ti= ti*10^-3;%[ms] doba impulzu    %  o o o| . |o o o |
% rsl - [mm] vnitrni poloměr civky %  <-rs2->.       
% rv -  [mm] poloměr vodiče        %   rs1>--.< 
% x -  čas pro tvorbu grafu v civka_cal.m   

inner_diameter = 40;    % mm

permeability=1.25664*10^-6; % permeabilita
f=8; % Hz
U=50; % V
rs1=inner_diameter/2; %mm
rs2=(N/(ls/2/rv))*rv*2+rs1; % mm

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
%%s

B = permeability*(N/(ls*10^-3))*I;  % T 
% P = I*U*f*ti;    % W 
P = (Im^2*(L*exp(-(2*ti*R)/L)*(4*exp((ti*R)/L) - 1) + 2*ti*R - 3*L))/2*f;

%% pro kresleni grafu v civka_cal.m
I2 = Im*(1-exp(-t/(L/R)));
B2 = permeability*(N/(ls*10^-3))*I2;

res.outer_diam = rs2*2;
res.current_max = I;
end
