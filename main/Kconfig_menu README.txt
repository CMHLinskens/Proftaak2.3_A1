Om menu's toe te voegen of aanpassen moet je de Kconfig openen.
Je moet een menu specificeren.
Nadat je een menu heeft aangemaakt of hebt kan je een submenu toevoegen.
In dit submenu komt de waardes te staan die je kan ophalen in je c file.
Als je bijvoobeeld "config NAME" zet, dan zal je NAME zien als een optie in de submenu.
Je moet per submenu een string, een default en een help geven.
De NAME wordt gebruikt in de code om de string waarde op te halen.
Als de gebruiker geen waarde geeft aan NAME dan zal het de default gebruiken.