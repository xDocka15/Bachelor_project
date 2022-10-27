clear
clc

N= 1000;
ls=120;
ti = 5;
rs1 = 15;
rv = 0.3;
t = 1;
[B,P]= civka_f(N,ls,ti,rv,t);
B
P
%%
% x = 1:1:100;
% figure;
% for i = 1:10
%     I=50/i;
% y = I*(1-exp(-(x*10^-3)/(0.023/i)));
% plot(x,y)
% hold on
% end

% %% zavislost B na vodici
% t = 0:0.0001:0.001;
% figure(1);
% subplot(2,1,1);
% for rv = 0.1:0.1:0.5
% 
%     [B,P,B2]= civka_f(N,ls,ti,rv,t);
%      plot(t,B2)
%  hold on
% end
% title('zavislost B na vodici')
% xlabel('time s')
% ylabel('B v T')
% 
% %% zavislost B na zavitech
% rv = 0.3;
% t = 0:0.0001:0.001;
% %figure;
% for N = 200:200:1000
% 
%     [B,P,B2]= civka_f(N,ls,ti,rv,t);
%     subplot(2,1,2);
%      plot(t,B2)
%  hold on
% end
% title('zavislost B na zavitech')
% xlabel('time s')
% ylabel('B v T')


%% zavislost B na prumeru vodice

dv = 0.1:0.1:0.5;
B = nan(length(dv),1);
P = nan(length(dv),1);

for i = 1:length(dv)
    [B(i),P(i),~]= civka_f(N,ls,ti,dv(i)/2,0);
end

figure(2);
plot(dv,B);
hold on;
plot(dv,P);
hold off;
title('Zavislost B na vodici')
xlabel('dv[mm]')
ylabel('B[T], P[W]')
legend('B','P')

%% zavislost B na delce impulzu

t_imp = 5:5:100;
B = nan(length(dv),1);
P = nan(length(dv),1);

for i = 1:length(t_imp)
    [B(i),P(i)]= civka_f(N,ls,t_imp(i),rv,0);
end

figure(3);
plot(t_imp,B);
hold on;
plot(t_imp,P);
hold off;
title('Zavislost B na delce impulzu')
xlabel('t_imp[ms]')
ylabel('B[T], P[W]')
legend('B','P')

%% vykresleni grafu pro zmenu prumeru vodice a poctu zavitu
delka_impulsu = 20; % ms
prumer_vodice = 1.15; % mm
pocet_zavitu = 4000;

[Prumery_vodice, Pocty_zavitu] = meshgrid(0.5:0.1:2,200:50:10000);   % Generate x and y data
kriterium = nan(size(Prumery_vodice));
for i = 1:numel(kriterium)
    kriterium(i) = kriterium_civka(Pocty_zavitu(i),delka_impulsu,Prumery_vodice(i));
end

figure(4);
surf(Prumery_vodice,Pocty_zavitu,kriterium);
xlabel('prumer vodice [mm]')
ylabel('pocet zavitu [-]')


%% vykresleni grafu pro delku impulzu a prumer vodice
[Prumery_vodice, Delky_impulsu] = meshgrid(0.5:0.1:2,0.5:0.5:200);   % Generate x and y data
kriterium = nan(size(Prumery_vodice));
for i = 1:numel(kriterium)
    kriterium(i) = kriterium_civka(pocet_zavitu,Delky_impulsu(i),Prumery_vodice(i));
end

figure(5);
surf(Prumery_vodice,Delky_impulsu,kriterium);
xlabel('prumer vodice [mm]')
ylabel('delky impulsu [ms]')


%% vykresleni grafu pro delku impulzu a pocet zavitu

[Pocty_zavitu, Delky_impulsu] = meshgrid(100:50:5000, 0.5:0.5:200);   % Generate x and y data
kriterium = nan(size(Pocty_zavitu));
for i = 1:numel(kriterium)
    kriterium(i) = kriterium_civka(Pocty_zavitu(i),Delky_impulsu(i),prumer_vodice);
end

figure(6);
surf(Pocty_zavitu,Delky_impulsu,kriterium);
xlabel('pocty zavitu [mm]')
ylabel('delky impulsu [ms]')

%% optimum pomoci optimalizace
delka_impulsu = 30; % ms
prumer_vodice = 0.8; % mm
pocet_zavitu = 4000;

fun = @(x) -1*kriterium_civka(x(1),x(2),x(3),1);

x0 = [pocet_zavitu, delka_impulsu, prumer_vodice];  % pocatecni bod optimalizace
options = optimoptions('fminunc','Algorithm','quasi-newton');
options.Display = 'iter';
options.MaxFunctionEvaluations = 10e3;

[x, fval, exitflag, output] = fminunc(fun,x0,options);  % vypocet optimalizace


optim_delka_impulsu = x(2);
optim_prumer_vodice = x(3);
optim_pocet_zavitu = x(1);

fprintf("Optimalni delka impulsu = %.1f, prumer vodice = %.2f, pocet zavitu = %.0f\n", ...
    optim_delka_impulsu, optim_prumer_vodice, optim_pocet_zavitu);


%% kriterialni funkce

function kriterium = kriterium_civka(pocet_zavitu,delka_impulsu,prumer_vodice,limited)
    if nargin < 4
        limited = 1;
    end
    delka_civky = 120; % mm
    P_civka = 15; % W
    D_civka_max = 100; % mm    

    % kriterium se bude maximalizovat
    % je primo umerne B - chceme maximalizovat
    % zmensuje se kdyz se vykon na civce vzdaluje od zadane hodnoty P_civka
    % zasadne se zmensuje pokud velikost civky prekroci D_civka_max

    [B,P,~,res] = civka_f(pocet_zavitu,delka_civky,delka_impulsu,prumer_vodice/2,0);

%     kriterium = 1000*B - (P>P_civka)*(10*P)^2 - (res.outer_diam > D_civka_max) * res.outer_diam^4;
    kriterium = 1000*B - 2*(P-P_civka)^2;
    if (kriterium < -100) && (limited)
        kriterium = -100;
    end
end


