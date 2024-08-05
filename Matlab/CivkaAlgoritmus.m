clear all
close all
clc

n = 0; % počítadlo kombinace
y = []; % pro uložení kombinace
z = 1;

%% projede kombinace, uloží pouze okoli požadovaneho vykonu
for N = 100:100:3000      % zavity 
    for ls = 70:10:100     % delka civky 
        for ti = 5:5:30       % čas impulzu 
            for rv = 0.1:0.1:1.0 % poloměr vodiče 
                [B,P,~,res]= CivkaFunkce(N,ls,ti,rv,1); % vypočet 
                if P >= 12 && P <= 16  % uloží if 12<P<16 
                    n = n+1;            % přičíst počítadlo 
                    y(n,:) = [n,B,P,N,ls,ti,rv,res.outer_diam]; % uložit paramtery 
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
plot(nres,(Bres./Pres),'o') % graf B/P
title('B/P')
xlabel('číslo měření')
ylabel('B/P')

%   z grafu se zjístí číslo kombinace vztupu, 
%   které jsou zapsány v proměnné 'y' 
res = [];
for i = 1:n % ulozi vysledky pro nejlepší kombinace do res 
    if (Bres(i,:)/Pres(i,:)) > 0.8
        res(z,:) = y(i,:); 
        z = z+1; 
    end 
end 
res_sorted = sortrows(res,2,'descend');
res_sorted(1,:)
%   n       B       P       N       ls      ti      rv  
