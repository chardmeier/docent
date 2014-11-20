
SRC=de
TRG=en
LANGPAIR = $(SRC)-$(TRG)
CORPUS = Europarl


test.000000000.xml:
	../../DEBUG/lcurve-docent -d SelectedWordLM -n input.xml config.xml test


fetch-corpus: 	$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz \
		$(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz \
		$(CORPUS)/$(LANGPAIR)/ids.gz \
		$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz \
		$(CORPUS)/raw/$(SRC) \
		$(CORPUS)/raw/$(TRG)

$(CORPUS)/$(LANGPAIR)/c.true.$(SRC).gz $(CORPUS)/$(LANGPAIR)/c.true.$(TRG).gz $(CORPUS)/$(LANGPAIR)/ids.gz:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/$(LANGPAIR)/$(notdir $@) )

$(CORPUS)/$(LANGPAIR)/aligned.grow-diag-final-and.gz:
	mkdir -p $(dir $@)
	( cd $(dir $@); wget http://opus.lingfil.uu.se/$(CORPUS)/wordalign/${LANGPAIR}/model/$(notdir $@) )

$(CORPUS)/raw/$(SRC) $(CORPUS)/raw/$(TRG):
	wget http://opus.lingfil.uu.se/$(CORPUS)/$(notdir $@).raw.tar.gz
	tar -xf $(notdir $@).raw.tar.gz
	rm -f $(notdir $@).raw.tar.gz

