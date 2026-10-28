// HOT Info Orbs kit sign-up form. Paste into a new Apps Script project
// (script.google.com) and run buildKitSignupForm. It creates the form and a
// linked responses Sheet in the running account's Drive, then logs the links.
// Running it again makes a second form; edit the existing one in Forms instead.
// Built 2026-10-28; live form:
// https://docs.google.com/forms/d/e/1FAIpQLSewJmwJH4fZf1O3rndbRK9dUFfkHtH9RUs-GQdB2Rw4AMjylw/viewform

function buildKitSignupForm() {
  const form = FormApp.create('HOT Info Orbs: Kit Sign-up');
  form.setDescription(
    'Five round displays on your desk showing the time, the weather and the markets. ' +
    'It updates over Wi-Fi and you set it up from any browser.\n\n' +
    'The club build starts in January. Build night is through-hole soldering only, one evening. ' +
    'Each kit has the board, a XIAO ESP32-S3 or S3 Plus, five displays, sockets, buttons and a USB-C cable.\n\n' +
    'Feature sheet: https://scottdube.github.io/hot-info-orbs/features.html\n\n' +
    "We'll email the final price and how to pay before parts are ordered.");
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
    .setChoiceValues(['1', '2', '3'])
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle('Module')
    .setHelpText('Either one fits the board. The Plus has 16 MB of flash; the standard S3 has 8 MB.')
    .setChoiceValues(['XIAO ESP32-S3 (8 MB)', 'XIAO ESP32-S3 Plus (16 MB)', 'No preference'])
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle('Enclosure')
    .setHelpText('The 3D-printed enclosure is still a design.')
    .setChoiceValues(["I'll print my own", "I'd like one printed for me", 'No enclosure'])
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle('Soldering experience')
    .setHelpText('So we know how much help to plan for on build night.')
    .setChoiceValues(['None', 'Some', 'Comfortable'])
    .setRequired(true);

  form.addParagraphTextItem().setTitle('Questions or comments');

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
