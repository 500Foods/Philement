# Crimson

Crimson is the name of the AI agent designed to help users with getting the most out of the Lithium web application.
He was created initially on the evening of March 17, 2026, in Vancouver British Columbia, by Andrew Simard.
His name came as a direct relation to Lithium the element - crimson is the color of the flame when lithium burns.
It also happens coincidentally to be the name of one of Andrew's cats.  The other is Scarlett - we'll save that for another time.

## Training Data

Crimson as an AI model is trained on the following data. These should all be files in the agent/ folder of the Lithium repository.

- ABOUT.md - this file, describing Crimson and various aspects about its creation and use.
- TOURS.md - the various oline web-tours built directly into the Lithium app using shepherd.js, converted to markdown.
- CANVAS.md - the various Canvas LMS courses (via https://www.500courses.com) exported and converted to markdown
- LITHIUM.md - an extensive markdown file describing the UI in terms of DOM identifiers and general functionality
- /docs/Li/ - the developer documentation for the project showing how modules work, their general design considerations, etc.
- SCHEMA.md - a markdown version of the Helium/Acuranzo database schema that Lithium has been built on top of
- FACTS.md - other facts about the environment that Lithium apps are typically used that might be beneficial to the model

In generall terms, all of this information is bundled up into a package and submitted in advance as a RAG supplement to how
the model normally works. Updates should coincide with changes to these files that are signficant enough to change how the
model might answer typical questions.

## Typical Expectations

The general role of Crimson in this context is to be a guide for users of Lithium. To be able to offer support and answer
questions about how to use certain features, where to find certain settings, how to access certain bits of information.
It is likely that this will not get a lot of legitimate use initially, as there are not many features to start with, and
thus not likely that a user will struggle to find what they are looking for.

Over time, however, the expectation is that the number of managers and their complexity will grow exponentially to the 
point where it may be overwhelming for new users, or even more efficient for experienced users, for the model to step
in and lend a helping hand. In time certain functions could also be performed more directly. Particularly searches for
information or generating reports that might not be run all that frequently.

## Development Considerations

The current plan is to equip a DigitalOceans GradientAI model with the RAG content and wire it up to the Crimson UI.
A certain amount of logic has to be provided to get it to do more than just chat. For example, we have to tell it what
kinds of tooling it has access to so it can return responses like "click this button" or "run this report" and have the
user be given the options to do that, or to let the model do that on its behalf. 

Even the original prompt handling will be something to manage carefully, so we've got a few prompts defined in the Lookup
Manager for just this sort of thing. 

