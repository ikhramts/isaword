/*
 * Copyright 2011 Iouri Khramtsov.
 *
 * This software is available under Apache License, Version 
 * 2.0 (the "License"); you may not use this file except in 
 * compliance with the License. You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
 
var nextWordIndex = 0;

function reloadWordsFromServer(refreshWord) {
    //Figure out what kind of words we need to get.
    var queryUrl = "/words/owl2/40/";
    var wordTypeElement = $('#word-type input.word-type:checked');
    var wordType = wordTypeElement.val();
    
    if (wordType == "length") {
        var fromLength = Math.max($('#from-length').val(), 2);
        var toLength = Math.min(Math.max($('#to-length').val(), fromLength), 15);
        queryUrl += "length/" + fromLength + "/" + toLength;
    
    } else {
        queryUrl += "index/" + wordType;
    }
    
    //Get the words.
    var doRefresh = refreshWord;
    
    $.getJSON(queryUrl, function(data, textStatus) {
        words = data;
        nextWordIndex = 0;
        
        if (doRefresh) {
            var wordContainer = $('#words-to-guess div:first span');
            $('#words-to-guess div:first span').fadeTo(1, 0.01);
            showNextWord();
            $('#words-to-guess div:first .word span').fadeTo(300, 1);
        }
    });
}

function showNextWord() {
    var wordContainer = $('#words-to-guess div.float-wrap:first');
    var nextWord = words[nextWordIndex];
    wordContainer.find('.word span').text(nextWord.word);
    
    var isValidElement = wordContainer.find('.is-a-word span');
    isValidElement.hide();
    //isValidElement.fadeTo(0, 0.001);
    
    if (nextWord.is_real) {
        isValidElement.text("valid");
        isValidElement.addClass("is-valid");
    
    } else {
        isValidElement.text("not valid");
        isValidElement.removeClass("is-valid");
    }
    
    var wordDescription = wordContainer.find('.description span');
    wordDescription.text(nextWord.description);
    //wordDescription.fadeTo(0, 0.001);
    wordDescription.hide();
    
    // Advance to the next word.
    nextWordIndex++;
    
    if (nextWordIndex == words.length) {
        reloadWordsFromServer();
    }
}

function processGuess(guess) {
    var wordContainer = $('#words-to-guess div.float-wrap:first');
    
    //Check whether the guess was correct.
    var isValid = (wordContainer.find('.is-a-word span').text() == "valid");
    
    if ((guess == "Yes" && isValid) || (guess == "No" && !isValid)) {
        wordContainer.find('.was-correct').addClass('true');
        
    } else if ((guess == "Yes" && !isValid) || (guess == "No" && isValid)) {
        wordContainer.find('.was-correct').addClass('false');
    }
    
    // Display the actual answer.
    wordContainer.find('.was-correct').fadeTo(400, 1);
    wordContainer.find('.is-a-word span').fadeTo(400, 1);
    wordContainer.find('.description span').fadeTo(400, 1);
    
    // Insert a new word.
    wordContainer.clone().hide().prependTo($('#words-to-guess'));
    showNextWord();
    var newWordContainer = $('#words-to-guess div.float-wrap:first');
    newWordContainer.find('.was-correct').removeClass('true').removeClass('false');
    //newWordContainer.attr('style', 'opacity: 0.01; display: none; filter: alpha(opacity=0.001); ZOOM: 1;');
    //newWordContainer.fadeTo(0, 0.01);
    //newWordContainer.slideUp(1);
    newWordContainer.find('.word span').fadeTo(1, 0.001);
    newWordContainer.slideDown(120);
    newWordContainer.find('.word span').fadeTo(400, 1);
    
    removeOldGuesses();
}

function removeOldGuesses() {
    var maxOldGuesses = $('#history-size').val();
    maxOldGuesses = Math.max(maxOldGuesses, 1);
    $('#words-to-guess div.float-wrap:gt(' + maxOldGuesses + ')').remove();
}

$(document).ready(function(){
    showNextWord();
    
    $('#guess a').click(function() {
        processGuess($(this).text());
        return false;
    });
    
    $('#word-type input.word-type, #from-length, #to-length').change(function() {
        reloadWordsFromServer(true /* refresh the next word */);
    });
    
    $('#history-size').change(function() {
        removeOldGuesses();
    });
});