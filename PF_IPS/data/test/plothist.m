dist = dat(:,5);
sid = dat(:,3);
rssi = dat(:,2);

cordat = {};

sidu = unique(sid);
for sidui=1:length(sidu)
   distf = dist(sid==sidu(sidui));
   rssif = rssi(sid==sidu(sidui));
   distfu = sort(unique(distf));
   cordat{sidui} = zeros(length(distfu),2);
   for distfui=1:length(distfu)
      rssiff = rssif(distf==distfu(distfui));
      cordat{sidui}(distfui,:) = [distfu(distfui) mean(rssiff)];
      %figure;
      %hist(rssiff,[-100:2:0]);
      %title(['Sensor ' num2str(sidu(sidui)) ' dist ' num2str(distfu(distfui))]);
   end
end
%%
figure;
ColOrd = get(gca, 'ColorOrder');
hold on;
for pi=1:length(cordat)
     plot(cordat{pi}(:,1),cordat{pi}(:,2),'.-','Color',ColOrd(mod(pi,length(ColOrd))+1,:));
end

%%
