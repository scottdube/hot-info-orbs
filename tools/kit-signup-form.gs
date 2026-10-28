// HOT Info Orbs kit sign-up form. Paste into a new Apps Script project
// (script.google.com) and run buildKitSignupForm. It creates the form and a
// linked responses Sheet in the running account's Drive, then logs the links.
// Running it again makes a second form; edit the existing one in Forms instead.
// Built 2026-10-28; live form:
// https://docs.google.com/forms/d/e/1FAIpQLSewJmwJH4fZf1O3rndbRK9dUFfkHtH9RUs-GQdB2Rw4AMjylw/viewform

const LIVE_FORM_ID = '1MMD9PAcmM4eElS5t4GJQU5yMvdGBJt0DDh9DqYVrQhc';

const DESCRIPTION =
  'Five round displays on your desk showing the time, the weather and the markets. ' +
  'It updates over Wi-Fi and you set it up from any browser.\n\n' +
  'The club build starts in January. Build night is through-hole soldering only, one evening.\n\n' +
  "WHAT'S IN THE KIT\n" +
  '\u2022 The carrier board\n' +
  '\u2022 Five 1.28-inch round displays\n' +
  '\u2022 Sockets for the displays and the XIAO, plus header pins\n' +
  '\u2022 Three buttons\n' +
  '\u2022 Two capacitors and a resistor\n\n' +
  'YOU SUPPLY\n' +
  '\u2022 A Seeed XIAO ESP32-S3 or XIAO ESP32-S3 Plus. Either fits; the Plus has more flash.\n' +
  "\u2022 A USB-C data cable. A charge-only cable won't load the firmware.\n\n" +
  'Feature sheet: https://scottdube.github.io/hot-info-orbs/features.html\n\n' +
  "We'll email the final price and how to pay before parts are ordered.";

const RENDER_URL =
  'https://raw.githubusercontent.com/scottdube/hot-info-orbs/main/docs/assets/enclosure-concept-3-winged-speedster.png';
const RENDER_TITLE = 'The Winged Speedster enclosure: a concept rendering, still a design';

const KIT_COUNTS = ['1', '2', '3', '4'];
const ENCLOSURE_CHOICES =
  ["I'll print my own", "I'll need to find someone to print it for me", 'No enclosure'];

function addRender_(form) {
  const blob = UrlFetchApp.fetch(RENDER_URL).getBlob();
  const image = form.addImageItem().setImage(blob).setTitle(RENDER_TITLE).setAlignment(FormApp.Alignment.CENTER);
  form.moveItem(image.getIndex(), 0);
}

function buildKitSignupForm() {
  const form = FormApp.create('HOT Info Orbs: Kit Sign-up');
  form.setDescription(DESCRIPTION);
  form.setConfirmationMessage(
    "Thanks, you're on the list. We'll email the final price and how to pay before parts are ordered.");
  form.setAllowResponseEdits(true);
  form.setProgressBar(false);

  form.addTextItem().setTitle('Name').setRequired(true);

  form.addTextItem()
    .setTitle('Email')
    .setRequired(true)
    .setValidation(FormApp.createTextValidation()
      .requireTextIsEmail()
      .setHelpText('Please enter a valid email address.')
      .build());

  form.addTextItem().setTitle('Phone').setHelpText('Optional.');

  form.addListItem()
    .setTitle('How many kits?')
    .setChoiceValues(KIT_COUNTS)
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle('Enclosure')
    .setHelpText('The 3D-printed enclosure is still a design.')
    .setChoiceValues(ENCLOSURE_CHOICES)
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle('Soldering experience')
    .setHelpText('So we know how much help to plan for on build night.')
    .setChoiceValues(['None', 'Some', 'Comfortable'])
    .setRequired(true);

  form.addParagraphTextItem().setTitle('Questions or comments');

  addRender_(form);

  // A Workspace account can default new forms to its own domain; members
  // sign up from personal addresses, so open it to anyone with the link.
  try { form.setRequireLogin(false); } catch (e) { Logger.log('setRequireLogin: ' + e); }
  // Forms made from a script may start unpublished.
  try { form.setPublished(true); } catch (e) { Logger.log('setPublished: ' + e); }

  const sheet = SpreadsheetApp.create('HOT Info Orbs: Kit Sign-up (Responses)');
  form.setDestination(FormApp.DestinationType.SPREADSHEET, sheet.getId());

  Logger.log('Share:     ' + form.getPublishedUrl());
  Logger.log('Edit:      ' + form.getEditUrl());
  Logger.log('Responses: ' + sheet.getUrl());
}

// One-off, 2026-10-28: bring the live form to the design above without
// changing its link. Kit contents listed, XIAO and USB cable now
// builder-supplied (so the Module question goes), 4 kits max, softer
// enclosure wording, enclosure render at the top.
function updateLiveForm() {
  const form = FormApp.openById(LIVE_FORM_ID);
  form.setDescription(DESCRIPTION);

  form.getItems().forEach(function (item) {
    const title = item.getTitle();
    if (title === 'Module') {
      form.deleteItem(item);
    } else if (title === 'How many kits?') {
      item.asListItem().setChoiceValues(KIT_COUNTS);
    } else if (title === 'Enclosure') {
      item.asMultipleChoiceItem().setChoiceValues(ENCLOSURE_CHOICES);
    }
  });

  if (form.getItems(FormApp.ItemType.IMAGE).length === 0) addRender_(form);

  form.getItems().forEach(function (item, i) { Logger.log(i + ' ' + item.getType() + ' ' + item.getTitle()); });
}
