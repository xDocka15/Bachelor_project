clear all
close all
clc

n = 0; % počítadlo kombinace
y = []; % pro uložení kombinace
z = 1;
res = [];

%% projede kombinace, uloží pouze okoli požadovaneho vykonu
for N = 200:200:3000      % zavity 
    for ls = 100:10:150     % delka civky 
        for ti = 1:10       % čas impulzu 
            for rv = 0.1:0.1:0.5 % poloměr vodiče 
                [B,P]= civka_f_old(N,ls,ti,rv); % vypočet 
                if P >= 12 && P <= 16  % uloží if 12<P<16 
                    n = n+1;            % přičíst počítadlo 
                    y(n,:) = [n,B,P,N,ls,ti,rv]; % uložit paramtery 
                end 
            end 
        end 
    end 
end 

%% graf 
nres=y(:,1); % číslo výsledku - X osa pro graf 
Bres=y(:,2); % 
Pres=y(:,3); %  

Bres = Bres/max(Bres); % normalizace 
Pres = Pres/max(Pres); % normalizace 

figure; 
plot(nres,(Bres./Pres)) % graf B/P
title('B/P')
xlabel('číslo měření')
ylabel('B/P')

%   z grafu se zjístí číslo kombinace vztupu, 
%   které jsou zapsány v proměnné 'y' 

for i = 1:n % ulozi vysledky pro nejlepší kombinace do res 
    if (Bres(i,:)/Pres(i,:)) > 0.95 
        res(z,:) = y(i,:); 
        z = z+1; 
    end 
end 
res_sorted = sortrows(res,2,'descend');
res_sorted(1,:)
%   n       B       P       N       ls      ti      rv  
